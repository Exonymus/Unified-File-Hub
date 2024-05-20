from datetime import timedelta

import sqlalchemy
from fastapi import APIRouter
from fastapi import Depends, HTTPException, status
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from passlib.context import CryptContext
from pydantic import EmailStr
from sqlalchemy.orm import Session
from typing_extensions import Annotated

import security.token as security
import cruds.user as crud
from database import get_db
from env import JWT_EXPIRE
from schemas import User, UserMetadata, Token
from schemas import (EmailUpdateRequest, PasswordUpdateRequest,
                     SQUpdateRequest, RecoverRequest)

pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")
oauth2_scheme = OAuth2PasswordBearer(tokenUrl="users/signin")

router = APIRouter()


@router.post('/signup', summary="Create new user", status_code=status.HTTP_201_CREATED)
async def create_user(metadata: UserMetadata, db: Session = Depends(get_db)):
    try:
        db.begin()
        crud.create_user(metadata=metadata, db=db)
        db.commit()
    except sqlalchemy.exc.IntegrityError:
        raise HTTPException(status_code=status.HTTP_409_CONFLICT,
                            detail=f"Username or email is already in use.")
    except HTTPException as http_err:
        db.rollback()
        raise http_err
    except Exception as e:
        db.rollback()
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to create user: {str(e)}")
    finally:
        db.close()

    return {"result": "success"}


@router.post("/signin")
async def login_for_access_token(
        form_data: Annotated[OAuth2PasswordRequestForm, Depends()],
        db: Session = Depends(get_db)
) -> Token:
    try:
        user = crud.authenticate_user(username=form_data.username, password=form_data.password, db=db)
        if not user:
            raise HTTPException(
                status_code=status.HTTP_401_UNAUTHORIZED,
                detail="Incorrect username or password",
                headers={"WWW-Authenticate": "Bearer"},
            )
        if user.is_banned:
            raise HTTPException(status_code=status.HTTP_403_FORBIDDEN,
                                detail="Banned user")
        access_token_expires = timedelta(minutes=int(JWT_EXPIRE))
        access_token = security.create_access_token(
            data={"sub": user.username}, expires_delta=access_token_expires
        )
        return Token(access_token=access_token, token_type="bearer")
    except HTTPException as http_err:
        raise http_err
    except Exception as e:
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to create access token: {str(e)}")


@router.get("/get_info", response_model=User)
async def get_user_info(
        current_user: Annotated[User, Depends(security.get_current_active_user)],
):
    return current_user


@router.post("/edit_email")
async def edit_user_email(
        current_user: Annotated[User, Depends(security.get_current_active_user)],
        update_metadata: EmailUpdateRequest,
        db: Session = Depends(get_db)
):
    try:
        db.begin()
        crud.update_user_email(user_id=current_user.id, email=update_metadata.email, db=db)
        db.commit()
    except HTTPException as http_err:
        db.rollback()
        raise http_err
    except Exception as e:
        db.rollback()
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to edit user: {str(e)}")
    finally:
        db.close()

    return {"result": "success"}


@router.post("/edit_password")
async def edit_user_password(
        current_user: Annotated[User, Depends(security.get_current_active_user)],
        update_metadata: PasswordUpdateRequest,
        db: Session = Depends(get_db)
):
    try:
        db.begin()
        crud.update_user_password(user_id=current_user.id, password=update_metadata.password, db=db)
        db.commit()
    except HTTPException as http_err:
        db.rollback()
        raise http_err
    except Exception as e:
        db.rollback()
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to edit user: {str(e)}")
    finally:
        db.close()

    return {"result": "success"}


@router.post("/edit_secret")
async def edit_user_secret(
        current_user: Annotated[User, Depends(security.get_current_active_user)],
        update_metadata: SQUpdateRequest,
        db: Session = Depends(get_db)
):
    try:
        db.begin()
        crud.update_user_secret(user_id=current_user.id, secret_num=update_metadata.secret_num,
                                secret_answer=update_metadata.secret_answer, db=db)
        db.commit()
    except HTTPException as http_err:
        db.rollback()
        raise http_err
    except Exception as e:
        db.rollback()
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to edit user: {str(e)}")
    finally:
        db.close()

    return {"result": "success"}


@router.post("/recover")
async def recover_user(
        recover_metadata: RecoverRequest,
        db: Session = Depends(get_db)
):
    try:
        db.begin()
        crud.recover_user(recover_metadata=recover_metadata, db=db)
        db.commit()
    except HTTPException as http_err:
        db.rollback()
        raise http_err
    except Exception as e:
        db.rollback()
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to recover user: {str(e)}")
    finally:
        db.close()

    return {"result": "success"}
