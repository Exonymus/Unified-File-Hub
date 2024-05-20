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
    reg_date: datetime


class UserInDB(User):
    hashed_password: str


class UserMetadata(BaseModel):
    username: str
    email: EmailStr
    password: str
    secret_num: int
    secret_answer: str


class EmailUpdateRequest(BaseModel):
    email: EmailStr


class PasswordUpdateRequest(BaseModel):
    password: str


class SQUpdateRequest(BaseModel):
    secret_num: int
    secret_answer: str


class RecoverRequest(BaseModel):
    username: str
    email: EmailStr
    password: str
    secret_num: int
    secret_answer: str
