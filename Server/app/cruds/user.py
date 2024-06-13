from datetime import datetime, timezone, timedelta
from typing import List, Union
from fastapi import HTTPException, status
from uuid import UUID, uuid4
from sqlalchemy.orm import Session
from sqlalchemy import and_
from passlib.context import CryptContext
from pydantic import EmailStr

from models import User as UserTable, Role, GDConn, FTPConn
from schemas import UserInDB, UserMetadata, RecoverRequest
from schemas import ConnectAPIData, ConnectFTPData

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


def update_user_email(user_id: UUID, email: EmailStr, db: Session) -> None:
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


def update_user_password(user_id: UUID, password: str, db: Session) -> None:
    """
        Update user password in the database.

        Args:
            user_id (UUID): ID of the user to be updated.
            password (str): Updated password.
            db (Session): The database session.
    """
    user = db.query(UserTable).get(user_id)
    if not user:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND,
                            detail="User not found.")

    hashed_password = get_password_hash(password)
    setattr(user, "hashed_password", hashed_password)


def update_user_secret(user_id: UUID, secret_num: int,
                       secret_answer: str, db: Session) -> None:
    """
        Update user secret question in the database.

        Args:
            user_id (UUID): ID of the user to be updated.
            secret_num (int): Updated secret num.
            secret_answer (str): Updated secret answer.
            db (Session): The database session.
    """
    user = db.query(UserTable).get(user_id)
    if not user:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND,
                            detail="User not found.")

    secret_answer_hash = get_password_hash(secret_answer)

    setattr(user, "secret_num", secret_num)
    setattr(user, "secret_answer", secret_answer_hash)


def recover_user(recover_metadata: RecoverRequest, db: Session) -> None:
    """
        Recover user password using provided metadata.

        Args:
            recover_metadata (RecoverRequest): Metadata to recover user's password.
            db (Session): The database session.
    """
    username: str = recover_metadata.username
    user = db.query(UserTable).filter(UserTable.username == username).first()

    if not user:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="User not found."
        )

    if user.email != recover_metadata.email \
            or user.secret_num != recover_metadata.secret_num \
            or not verify_password(recover_metadata.secret_answer, user.secret_answer):
        raise HTTPException(
            status_code=status.HTTP_406_NOT_ACCEPTABLE,
            detail="Bad user credentials."
        )

    hashed_password = get_password_hash(recover_metadata.password)

    setattr(user, "hashed_password", hashed_password)


def add_google_connection(user_id: UUID, data: ConnectAPIData, db: Session) -> None:
    """
        Add Google Drive connection using provided metadata.

        Args:
            user_id (UUID): ID of the user to be updated.
            data (ConnectAPIData): API-token to connect to the Google API.
            db (Session): The database session.
    """
    try:
        # Check existence
        gd_conn = db.query(GDConn).filter(GDConn.user_id == user_id).first()
        if gd_conn:
            db.delete(gd_conn)

        # Create a new Google Drive connection object with the provided metadata
        api_connection = GDConn(
            id=uuid4(),
            user_id=user_id,
            access_key=data.access_key,
            refresh_key=data.refresh_key
        )

        # Add the new connection to the session
        db.add(api_connection)
    except Exception as e:
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to create gdrive connection: {e}")


def get_google_connection(user_id: UUID, db: Session) -> str:
    """
        Get Google Drive connection.

        Args:
            user_id (UUID): ID of the user, who owns api-key.
            db (Session): The database session.

        Returns:
            str: Api key if found.
    """
    # Find Google Drive connection by User id
    gd_conn = db.query(GDConn).filter(GDConn.user_id == user_id).first()

    if not gd_conn:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Api-key not found."
        )

    return str.join(",", [gd_conn.access_key, gd_conn.refresh_key])


def remove_google_connection(user_id: UUID, db: Session) -> None:
    """
        Remove user's Google Drive connection.

        Args:
            user_id (UUID): ID of the user to be updated.
            db (Session): The database session.
    """
    try:
        connection = db.query(GDConn).filter(GDConn.user_id == user_id).first()

        if not connection:
            raise HTTPException(status_code=status.HTTP_404_NOT_FOUND,
                                detail="Connection not found.")
        db.delete(connection)
    except Exception as e:
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to remove gdrive connection: {e}")


def add_ftp_connection(user_id: UUID, data: ConnectFTPData, db: Session) -> None:
    """
        Add FTP connection using provided metadata.

        Args:
            user_id (UUID): ID of the user to be updated.
            data (ConnectFTPData): API-token to connect to the Google API.
            db (Session): The database session.
    """
    try:
        # Create a new Google Drive connection object with the provided metadata
        ftp_connection = FTPConn(
            id=uuid4(),
            user_id=user_id,
            name=data.name,
            ftp_user=data.user,
            ftp_pass=data.password,
            ftp_ip=data.ip
        )

        # Add the new connection to the session
        db.add(ftp_connection)
    except Exception as e:
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to create ftp connection: {e}")


def get_ftp_connections(user_id: UUID, db: Session) -> List[FTPConn]:
    """
        Get FTP connections.

        Args:
            user_id (UUID): ID of the user, who owns api-key.
            db (Session): The database session.

        Returns:
            str: Api key if found.
    """
    # Find Google Drive connection by User id
    ftp_conns = db.query(FTPConn).filter(FTPConn.user_id == user_id).all()

    if not ftp_conns:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="FTP connections not found."
        )

    return ftp_conns


def remove_ftp_connection(user_id: UUID, conn_id: UUID, db: Session) -> None:
    """
        Remove user's FTP connection.

        Args:
            user_id (UUID): ID of the user to be updated.
            conn_id (UUID): ID of the connection to be removed.
            db (Session): The database session.
    """
    try:
        connection = db.query(GDConn).filter(and_(FTPConn.id == conn_id, FTPConn.user_id == user_id)).first()

        if not connection:
            raise HTTPException(status_code=status.HTTP_404_NOT_FOUND,
                                detail="Connection not found.")
        db.delete(connection)
    except Exception as e:
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to remove ftp connection: {e}")


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
