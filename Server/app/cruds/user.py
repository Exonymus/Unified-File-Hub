from datetime import datetime, timezone, timedelta
from typing import Union
from fastapi import HTTPException, status
from uuid import UUID, uuid4
from sqlalchemy.orm import Session
from passlib.context import CryptContext
from pydantic import EmailStr

from models import User as UserTable, Role
from schemas import UserInDB, UserMetadata


pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")


def create_user(metadata: UserMetadata, db: Session) -> None:
    """
        Create user metadata in the database.

        Args:
            metadata (UserMetadata): Metadata of the user to be created.
            db (Session): The database session.
    """
    try:
        # Create a new User object with the provided metadata
        user = UserTable(
            id=uuid4(),
            role_id=db.query(Role).filter(Role.name == "user").first().id,
            username=metadata.username,
            email=metadata.email,
            secret_num=metadata.secret_num,
            secret_answer=get_password_hash(metadata.secret_answer),
            hashed_password=get_password_hash(metadata.password),
            reg_date=datetime.now(tz=timezone(timedelta(hours=3))),
            is_banned=False
        )

        # Add the new user to the session
        db.add(user)
    except Exception as e:
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to create user: {e}")


def update_user(user_id: UUID, email: EmailStr, db: Session) -> None:
    """
        Update user email in the database.

        Args:
            user_id (UUID): ID of the user to be updated.
            email (EmailStr): Updated email.
            db (Session): The database session.
    """
    user = db.query(UserTable).get(user_id)
    if not user:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND,
                            detail="User not found.")

    setattr(user, "email", email)


def verify_password(plain_password: str, hashed_password: str) -> bool:
    """
        Verify user password with hash.

        Args:
            plain_password (str): Password to check.
            hashed_password (str): Encrypted password hash.

        Returns:
            bool: True if password valid, otherwise False.
    """
    return pwd_context.verify(plain_password, hashed_password)


def get_password_hash(password: str):
    """
        Hash password with bcrypt encryption

        Args:
            password (str): Password to encrypt.

        Returns:
            str: Password hash.
    """
    return pwd_context.hash(password)


def get_user(username: str, db: Session) -> 'UserInDB':
    """
        Get user's metadata from DB.

        Args:
            username (str): User's name to find metadata in DB.
            db (Session): The database session.

        Returns:
            UserInDB: User's metadata.
    """
    user = db.query(UserTable).filter(UserTable.username == username).first()
    db.close()
    if user:
        return UserInDB(**user.__dict__)
    else:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND,
                            detail="User not found.")


def authenticate_user(username: str, password: str, db: Session) -> Union['UserInDB', bool]:
    """
        Authorize user with username and password.

        Args:
            username (str): User's name.
            password (str): User's password.
            db (Session): The database session.

        Returns:
            UserInDB: User's metadata if provided data is correct, False otherwise.
    """
    try:
        user = get_user(username=username, db=db)
        if not user:
            return False
        if not verify_password(password, user.hashed_password):
            return False
        return user
    except HTTPException as http_err:
        raise http_err
    except Exception as e:
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to authenticate user: {str(e)}")
