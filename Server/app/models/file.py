import datetime
from datetime import datetime as dt, timezone, timedelta
import mimetypes
import os

import sqlalchemy
from database import Base
from sqlalchemy import Column, String, Boolean, DateTime, ForeignKey
from sqlalchemy.orm import relationship
from sqlalchemy_utils import UUIDType
from uuid import uuid4


class File(Base):
    __tablename__ = 'files'

    id = Column(UUIDType(binary=False), primary_key=True, default=uuid4)
    name = Column(String(255), nullable=False)
    path = Column(String(255), nullable=False)
    mime_type = Column(String(255), nullable=False)
    description = Column(String(255))
    author = Column(String(255))
    theme = Column(String(255))
    is_public = Column(Boolean, nullable=False)
    owner_id = Column(UUIDType(binary=False), ForeignKey("users.id"), nullable=False)
    upload_date = Column(DateTime(), default=dt.now(tz=timezone(timedelta(hours=3))), nullable=False)

    owner = relationship("User", back_populates="files")

    def to_json(self):
        return {
            "id": self.id,
            "name": self.name,
            "path": self.path,
            "mime_type": self.mime_type,
            "description": self.description,
            "author": self.author,
            "theme": self.theme,
            "is_public": self.is_public,
            "owner_id": self.owner_id,
            "upload_date": self.upload_date,
            "size":
                os.path.getsize(f"/usr/src/app/files/{self.owner_id}/files/{self.path}"
                                f"/{self.id}{mimetypes.guess_extension(self.mime_type)}")
        }

    def copy(self) -> 'File':
        """
        Create a copy of the file metadata

        Returns:
            File: The copied file object.
        """
        return File(
            id=uuid4(),
            name=self.name,
            path=self.path,
            mime_type=self.mime_type,
            description=self.description,
            author=self.author,
            theme=self.theme,
            is_public=self.is_public,
            owner_id=self.owner_id,
            upload_date=dt.now(tz=timezone(timedelta(hours=3)))
        )
