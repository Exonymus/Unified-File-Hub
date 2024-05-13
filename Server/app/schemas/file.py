from pydantic import BaseModel
from uuid import UUID


class File(BaseModel):
    name: str
    path: str
    mime_type: str
    description: str
    author: str
    theme: str
    is_public: bool

    class Config:
        orm_mode = True


class FileUpdate(BaseModel):
    name: str
    description: str
    author: str
    theme: str
    is_public: bool

    class Config:
        orm_mode = True
