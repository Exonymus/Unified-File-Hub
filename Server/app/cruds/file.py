import uuid
from datetime import datetime

from fastapi import HTTPException, UploadFile
from models import File
from schemas import FileMetadata
from sqlalchemy import select, or_, and_
from sqlalchemy.orm import Session
from starlette.status import HTTP_404_NOT_FOUND
from uuid import UUID


def create_file_metadata(db: Session, metadata: FileMetadata):
    file = File(
        filename=metadata.filename,
        download_id=metadata.download_id,
        author_name=metadata.author_name,
        publication_name=metadata.publication_name,
        theme=metadata.theme,
        publication_date=metadata.publication_date,
        description=metadata.description,
        upload_date=metadata.upload_date,
        uploader_name=metadata.uploader_name,
        folder_path=metadata.folder_path,
        doc_type=metadata.doc_type,
        is_public=metadata.is_public
    )
    db.add(file)
    db.commit()
    return 0


def copy_file(db: Session, file_id: int, folder_path: str, username: str):
    file = db.query(File).filter(File.id == file_id).first()
    newfile_id = uuid.uuid4()
    copy_file = File(filename=str(uuid.uuid4()) + file.publication_name, download_id=str(uuid.uuid4()),
                     author_name=file.author_name, publication_name=file.publication_name,
                     theme=file.theme, publication_date=file.publication_date, description=file.description,
                     upload_date=file.upload_date, uploader_name=username, doc_type=file.doc_type,
                     is_public=0, folder_path=folder_path)
    db.add(copy_file)
    db.commit()


def get_user_files(db: Session, username: str):
    query = select(File).where(or_(File.uploader_name == username, File.is_public == 1))
    result = db.execute(query)
    response = [row.to_json() for row in result.scalars()]
    return response


def get_user_files_metadata(db: Session, username: str):
    query = select(File).where(or_(File.uploader_name == username, File.is_public == 1))
    result = db.execute(query)
    response = [row.to_json_no_blob() for row in result.scalars()]
    return response


def delete_file(db: Session, file_id: int):
    file = db.query(File).filter(File.id == file_id).first()
    db.delete(file)
    db.commit()
    return 0


def update_file(db: Session, file_id: int, author_name: str, publication_name: str,
                theme: str, publication_date: datetime, description: str,
                is_public: bool, folder_path: str):
    file = db.query(File).filter(File.id == file_id).first()
    setattr(file, "author_name", author_name)
    setattr(file, "publication_name", publication_name)
    setattr(file, "theme", theme)
    setattr(file, "publication_date", publication_date)
    setattr(file, "description", description)
    setattr(file, "is_public", bool(int(is_public)))
    setattr(file, "folder_path", folder_path)
    db.commit()
    return 0


def get_all_public_files(db: Session):
    query = select(File).where(File.is_public == 1)
    result = db.execute(query)
    response = [row.to_json_no_blob() for row in result.scalars()]
    return response


def get_public_file_by_id(db: Session, file_id: int):
    query = select(File).where(and_(File.is_public == 1, File.id == file_id))
    result = db.execute(query)
    response = [row.to_json_no_blob() for row in result.scalars()]
    return response
