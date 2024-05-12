from pydantic import BaseModel, EmailStr


class User(BaseModel):
    username: str
    email: EmailStr
    secret_num: int
    secret_answer: str
    password: str

    class Config:
        orm_mode = True
