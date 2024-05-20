from pydantic import BaseModel


class File(BaseModel):
    name: str
    path: str
    mime_type: str
    description: str
    category: str
    tag: str
    is_public: bool

    class Config:
        orm_mode = True


class FileUpdate(BaseModel):
    name: str
    path: str
    description: str
    category: str
    tag: str
    is_public: bool

    class Config:
        orm_mode = True
