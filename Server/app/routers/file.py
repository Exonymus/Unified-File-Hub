import asyncio
import os
from datetime import datetime
from pathlib import Path
from uuid import UUID

import aiofiles
from schemas.file import FileMetadata
import cruds.file as crud
from database import get_db
from fastapi import APIRouter, Depends, UploadFile, File, HTTPException
from pydantic import EmailStr
from sqlalchemy.orm import Session

router = APIRouter()


@router.post('/get_user_files')
async def get_user_files(username: str, db: Session = Depends(get_db)):
    response = crud.get_user_files(username=username, db=db)
    response_json = {}
    counter = 0
    for item in response:
        response_json[f"{counter}"] = response[counter]
        counter += 1
    return {"data": response_json}


@router.post('/get_user_files_metadata')
async def get_user_files_metadata(username: str, db: Session = Depends(get_db)):
    response = crud.get_user_files_metadata(username=username, db=db)
    response_json = {}
    counter = 0
    for item in response:
        response_json[f"{counter}"] = response[counter]
        counter += 1
    return {"data": response_json}


@router.post('/delete_file')
async def delete_file(file_id: int, db: Session = Depends(get_db)):
    crud.delete_file(file_id=file_id, db=db)
    return {"data": 0}


@router.post('/update_file')
async def update_file(file_id: int, author_name: str, publication_name: str,
                      theme: str, publication_date: datetime, description: str,
                      is_public: bool, folder_path: str, db: Session = Depends(get_db)):
    crud.update_file(file_id=file_id, author_name=author_name, publication_name=publication_name,
                     theme=theme, publication_date=publication_date, description=description,
                     is_public=is_public, folder_path=folder_path, db=db)
    return {"data": 0}


async def save_uploaded_file(file_owner: str, file_path: str, file_name: str, file_data: UploadFile):
    destination = f"/usr/src/app/files/{file_owner}/files/{file_path}/"
    try:
        Path(destination).mkdir(parents=True, exist_ok=True)
        async with aiofiles.open(destination + file_name, 'wb') as out_file:
            while content := await file_data.read(1024):  # async read chunk
                await out_file.write(content)  # async write chunk
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to save file: {str(e)}")


@router.post('/upload_file')
async def upload_file(metadata: FileMetadata = Depends(),
                      input_data: UploadFile = File(...),
                      db: Session = Depends(get_db)):
    await asyncio.create_task(save_uploaded_file(file_owner=metadata.uploader_name,
                                                 file_path=metadata.folder_path,
                                                 file_name=metadata.filename_full,
                                                 file_data=input_data))
    crud.create_file_metadata(metadata=metadata, db=db)

    return {"data": 0}


@router.post('/copy_file')
async def copy_file(file_id: int, folder_path: str,
                    username: str, db: Session = Depends(get_db)):
    crud.copy_file(file_id=file_id, folder_path=folder_path,
                   username=username, db=db)


@router.post('/get_public_file_by_id')
async def get_public_file_by_id(file_id: int, db: Session = Depends(get_db)):
    return {"data": crud.get_public_file_by_id(file_id=file_id, db=db)}


@router.post('/get_all_public_files')
async def get_all_public_files(db: Session = Depends(get_db)):
    response = crud.get_all_public_files(db=db)
    response_json = {}
    counter = 0
    for item in response:
        response_json[f"{counter}"] = response[counter]
        counter += 1
    return {"data": response_json}
