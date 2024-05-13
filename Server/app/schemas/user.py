from datetime import datetime
from uuid import UUID
from pydantic import BaseModel, EmailStr


# class User(BaseModel):
#     username: str
#     email: EmailStr
#     secret_num: int
#     secret_answer: str
#     password: str
#
#     class Config:
#         orm_mode = True


class User(BaseModel):
    id: UUID
    username: str
    email: EmailStr
    secret_num: int
    secret_answer: str
    is_banned: bool


class UserInDB(User):
    reg_date: datetime
    hashed_password: str
