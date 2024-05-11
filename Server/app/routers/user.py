import cruds.user as crud
from database import get_db
from fastapi import APIRouter, Depends
from pydantic import EmailStr
from schemas.user import User as UserSchema
from sqlalchemy.orm import Session


router = APIRouter()


@router.post('/get_user_info')
async def get_user_info(username: str, db: Session = Depends(get_db)):
    response = crud.get_user_info(username=username, db=db)
    response_json = {}
    counter = 0
    for _ in response:
        response_json[f"{counter}"] = response[counter]
        counter += 1
    return {"data": response_json}


@router.post('/edit_user')
async def edit_user(user_id: int, email: EmailStr, db: Session = Depends(get_db)):
    crud.update_user(user_id=user_id, email=email, db=db)
    return {"data": 0}


@router.post('/auth_user')
async def auth_user(username: str, password: str, db: Session = Depends(get_db)):
    if crud.auth_user(username=username, password=password, db=db):
        return {"is_auth": True}
    return {"is_auth": False}
