from database import Base
from sqlalchemy import Column, Integer, String, Boolean, BLOB, DATETIME
from sqlalchemy_utils import UUIDType
from uuid import uuid4


class Files(Base):
    __tablename__ = 'files'

    id = Column(UUIDType(binary=False), primary_key=True, default=uuid4, nullable=False)
    filename = Column(String(255))
    download_id = Column(UUIDType(binary=False), default=uuid4, nullable=False)
    author_name = Column(String(255))
    publication_name = Column(String(255))
    theme = Column(String(255))
    publication_date = Column(DATETIME)
    description = Column(String(255))
    upload_date = Column(DATETIME, nullable=False)
    uploader_name = Column(String(255))
    folder_path = Column(String(255))
    doc_type = Column(String(255))
    is_public = Column(Boolean)
    data = Column(BLOB)

    def to_json(self):
        return {"id": self.id, "filename": self.filename, "download_id": self.download_id,
                "author_name": self.author_name, "publication_name": self.publication_name, "theme": self.theme,
                "publication_date": self.publication_date, "description": self.description,
                "upload_date": self.upload_date,
                "uploader_name": self.uploader_name, "folder_path": self.folder_path, "doc_type": self.doc_type,
                "is_public": self.is_public, "data": str(self.data)}

    def to_json_no_blob(self):
        return {"id": self.id, "filename": self.filename, "download_id": self.download_id,
                "author_name": self.author_name, "publication_name": self.publication_name, "theme": self.theme,
                "publication_date": self.publication_date, "description": self.description,
                "upload_date": self.upload_date,
                "uploader_name": self.uploader_name, "folder_path": self.folder_path, "doc_type": self.doc_type,
                "is_public": self.is_public, "file_size": len(self.data)}
