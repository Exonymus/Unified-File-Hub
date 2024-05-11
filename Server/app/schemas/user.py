# from datetime import datetime
from typing import List
from pydantic import BaseModel
from uuid import UUID


class User(BaseModel):
    id: int
    username: str
    email: str
    password: str
    on_active: bool
    is_banned: bool
    is_actual: bool
    role: int

    class Config:
        orm_mode = True
