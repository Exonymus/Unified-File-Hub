import asyncio
import json
import mimetypes
import os
import aiofiles
from pathlib import Path
from uuid import UUID
from typing_extensions import Annotated
from fastapi import APIRouter, Depends, UploadFile, File, HTTPException, status, Form
from fastapi.responses import FileResponse
from sqlalchemy.orm import Session

import cruds.file as crud
import security.token as security
from database import get_db
from schemas import User
from schemas import FileMetadata, FileUpdate

router = APIRouter()


async def save_uploaded_file(file_owner_id: UUID, file_path: str,
                             file_name: str, file_data: UploadFile):
    destination = f"/usr/src/app/files/{str(file_owner_id)}/files/{file_path}/"
    try:
        Path(destination).mkdir(parents=True, exist_ok=True)
        async with aiofiles.open(f"{destination}{file_name}", 'wb') as out_file:
            while content := await file_data.read(1024):
                await out_file.write(content)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to save file: {str(e)}")


@router.post('/upload_file', status_code=status.HTTP_201_CREATED)
async def upload_file(
        current_user: Annotated[User, Depends(security.get_current_active_user)],
        input_data: UploadFile = File(...),
        metadata: str = Form(...),
        db: Session = Depends(get_db)):
    try:
        metadata_dict = json.loads(metadata)
        db.begin()
        file_id, file_type = crud.create_file_metadata(metadata=FileMetadata(**metadata_dict),
                                                       user_id=current_user.id,
                                                       db=db)
        file_name = str(file_id) + mimetypes.guess_extension(file_type)
        await asyncio.create_task(save_uploaded_file(file_owner_id=current_user.id,
                                                     file_path=metadata_dict["path"],
                                                     file_name=file_name,
                                                     file_data=input_data))
        db.commit()
    except HTTPException as http_err:
        db.rollback()
        raise http_err
    except Exception as e:
        db.rollback()
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to upload file: {str(e)}")
    finally:
        db.close()

    return {"result": "success"}


@router.get('/get_user_files')
async def get_user_files(
        current_user: Annotated[User, Depends(security.get_current_active_user)],
        db: Session = Depends(get_db)):
    try:
        files = crud.get_user_files_metadata(user_id=current_user.id, db=db)
        return {"data": {index: file.to_json(db) for index, file in enumerate(files)}}
    except HTTPException as http_err:
        raise http_err
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to retrieve files: {str(e)}")


@router.post('/update_file/{file_id}')
async def update_file(
        current_user: Annotated[User, Depends(security.get_current_active_user)],
        file_id: UUID, metadata: FileUpdate, db: Session = Depends(get_db)):
    try:
        db.begin()
        crud.update_file_metadata(file_id=file_id, user_id=current_user.id,
                                  metadata=metadata, db=db)
        crud.remove_empty_directories(
            root_path=(Path("/usr/src/app/files") / str(current_user.id) / "files")
        )
        db.commit()
    except HTTPException as http_err:
        db.rollback()
        raise http_err
    except Exception as e:
        db.rollback()
        raise HTTPException(status_code=500, detail=f"Failed to update file: {str(e)}")
    finally:
        db.close()

    return {"result": "success"}


@router.post('/delete_file/{file_id}')
async def delete_file(
        current_user: Annotated[User, Depends(security.get_current_active_user)],
        file_id: UUID, db: Session = Depends(get_db)):
    try:
        db.begin()
        file_path = crud.delete_file_metadata(file_id=file_id, user_id=current_user.id, db=db)
        os.remove(file_path)
        crud.remove_empty_directories(
            root_path=(Path("/usr/src/app/files") / str(current_user.id) / "files")
        )
        db.commit()
    except HTTPException as http_err:
        db.rollback()
        raise http_err
    except Exception as e:
        db.rollback()
        raise HTTPException(status_code=500, detail=f"Failed to delete file: {str(e)}")
    finally:
        db.close()

    return {"result": "success"}


@router.get('/download_file/{file_id}')
async def download_file(
        current_user: Annotated[User, Depends(security.get_current_active_user)],
        file_id: UUID, db: Session = Depends(get_db)):
    try:
        file_metadata = crud.get_file_metadata_by_id(file_id=file_id, user_id=current_user.id, db=db)

        file_name = file_metadata.name
        file_type = file_metadata.mime_type
        file_full_path = crud.construct_file_path(user_id=file_metadata.owner_id, file_id=file_id,
                                                  file_path=file_metadata.path, file_type=file_type)

        return FileResponse(filename=file_name, media_type=file_type, path=file_full_path)

    except HTTPException as http_err:
        raise http_err

    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to download file: {str(e)}")


@router.post('/copy_file', status_code=status.HTTP_201_CREATED)
async def copy_file(
        current_user: Annotated[User, Depends(security.get_current_active_user)],
        file_id: UUID, file_path: str, db: Session = Depends(get_db)):
    try:
        db.begin()
        crud.copy_file(file_id=file_id, user_id=current_user.id, file_path=file_path, db=db)
        db.commit()
    except HTTPException as e:
        db.rollback()
        raise e
    except Exception as e:
        db.rollback()
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR, detail=f"Internal Server Error: {e}")
    finally:
        db.close()

    return {"result": "success"}

# @router.post('/get_public_file_by_id')
# async def get_public_file_by_id(file_id: UUID, db: Session = Depends(get_db)):
#     return {"data": crud.get_public_file_by_id(file_id=file_id, db=db)}
#
#
# @router.post('/get_all_public_files')
# async def get_all_public_files(db: Session = Depends(get_db)):
#     response = crud.get_all_public_files(db=db)
#     response_json = {}
#     counter = 0
#     for item in response:
#         response_json[f"{counter}"] = response[counter]
#         counter += 1
#     return {"data": response_json}
