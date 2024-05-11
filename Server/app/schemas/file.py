from datetime import datetime

from pydantic import BaseModel
from uuid import UUID


class FileMetadata(BaseModel):
    download_id: UUID
    filename: str
    filename_full: str
    author_name: str
    publication_name: str
    theme: str
    publication_date: datetime
    uploader_name: str
    description: str
    upload_date: datetime
    doc_type: str
    is_public: bool
    folder_path: str

    class Config:
        orm_mode = True
