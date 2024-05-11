import mimetypes
import os

import sqlalchemy
from database import Base
from filetype import filetype
from sqlalchemy import Column, Integer, String, Boolean, BLOB, DATETIME
from sqlalchemy_utils import UUIDType
from uuid import uuid4


class File(Base):
    __tablename__ = 'files'

    id = Column(Integer, primary_key=True, autoincrement=True, nullable=False)
    filename = Column(String(255))
    download_id = Column(UUIDType(binary=False), nullable=False)
    author_name = Column(String(255))
    publication_name = Column(String(255))
    theme = Column(String(255))
    publication_date = Column(sqlalchemy.DateTime())
    description = Column(String(255))
    upload_date = Column(sqlalchemy.DateTime(), nullable=False)
    uploader_name = Column(String(255))
    folder_path = Column(String(255))
    doc_type = Column(String(255))
    is_public = Column(Boolean)

    def to_json(self):
        return {"id": self.id, "filename": self.filename, "download_id": self.download_id,
                "author_name": self.author_name, "publication_name": self.publication_name, "theme": self.theme,
                "publication_date": self.publication_date, "description": self.description,
                "upload_date": self.upload_date,
                "uploader_name": self.uploader_name, "folder_path": self.folder_path, "doc_type": self.doc_type,
                "is_public": self.is_public, "data": str(self.data)}

    def to_json_no_blob(self):
        return {"id": self.id,
                "filename": self.filename,
                "download_id": self.download_id,
                "author_name": self.author_name,
                "publication_name": self.publication_name,
                "theme": self.theme,
                "publication_date": self.publication_date,
                "description": self.description,
                "upload_date": self.upload_date,
                "uploader_name": self.uploader_name,
                "folder_path": self.folder_path,
                "doc_type": self.doc_type,
                "is_public": self.is_public,
                "file_size":
                    os.path.getsize(f"/usr/src/app/files/{self.uploader_name}"
                                    f"/files/{self.folder_path}"
                                    f"/{self.publication_name}{mimetypes.guess_extension(self.doc_type)}")}
