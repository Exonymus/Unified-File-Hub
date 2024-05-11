import bcrypt
from fastapi import HTTPException
from models import User
from sqlalchemy import select
from sqlalchemy.orm import Session
from starlette.status import HTTP_404_NOT_FOUND
from uuid import UUID


def auth_user(username: str, password: str, db: Session):
    """Authenticate user with username and encrypted password"""
    user = db.query(User).filter(User.username == username).first()

    if not user:
        return False
    if not bcrypt.checkpw(password.encode('utf-8'), user.password.encode('utf-8')):
        return False

    return True


def get_user_info(username: str, db: Session):
    """Get user's metadata with username"""
    query = select(User).where(User.username == username)
    result = db.execute(query)
    response = [row.to_json() for row in result.scalars()]
    return response


def update_user(user_id: int, email: str, db: Session):
    """Update user's metadata with new email"""
    user = db.query(User).filter(User.id == user_id).first()
    setattr(user, "email", email)
    db.commit()
    return 0
