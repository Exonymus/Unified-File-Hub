import mimetypes
import os
import shutil
from pathlib import Path
from typing import List, Tuple, Union
from uuid import UUID, uuid4

from fastapi import HTTPException, status
from models import File, User
from schemas import FileMetadata, FileUpdate
from sqlalchemy import or_
from sqlalchemy.orm import Session


def construct_file_path(user_id: UUID, file_id: UUID,
                        file_path: str, file_type: str) -> Path:
    """
        Construct the full file path.

        Args:
            user_id (UUID): The user ID.
            file_id (UUID): The file ID.
            file_path (str): The path of the file within the user's directory.
            file_type (str): The MIME type of the file.

        Returns:
            Path: The full file path.
    """

    # Construct the base directory path
    base_path = Path("/usr/src/app/files")

    # Construct the user-specific directory path
    user_directory_path = base_path / str(user_id) / "files"

    # Ensure the directory structure exists
    user_directory_path.mkdir(parents=True, exist_ok=True)

    # Obtain file extension
    file_ext = mimetypes.guess_extension(file_type) or ""

    # Construct the full file path
    file_full_path = user_directory_path / file_path / f"{file_id}{file_ext}"

    return file_full_path


def remove_empty_directories(root_path: str) -> None:
    """
        Remove empty directories recursively starting from the root path.

        Args:
            root_path (str): The root directory path to start removing empty directories from.

        Returns:
            None
    """
    # Walk through the directory tree starting from the root_path
    for dir_path, dir_names, _ in os.walk(root_path, topdown=False):
        for dir_name in dir_names:
            directory_path = os.path.join(dir_path, dir_name)

            # Check if the directory is empty
            if not os.listdir(directory_path):
                try:
                    # Remove the empty directory
                    os.rmdir(directory_path)
                except ...:
                    pass


def create_file_metadata(metadata: FileMetadata, user_id: UUID, db: Session) -> Tuple[UUID, str]:
    """
    Create file metadata in the database.

    Args:
        metadata (FileMetadata): Metadata of the file to be created.
        user_id (UUID): ID of the user who uploaded the file.
        db (Session): The database session.

    Returns:
        tuple: A tuple containing the ID of the created file and MIME type.
    """
    try:
        # Create a new File object with the provided metadata
        file = File(**metadata.dict())
        file.owner_id = user_id
        file.id = uuid4()

        # Add the new file to the session and commit changes
        db.add(file)
        return file.id, file.mime_type
    except Exception as e:
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to create file metadata: {e}")


def get_user_files_metadata(user_id: UUID, db: Session) -> List[File]:
    """
    Retrieve files owned by a user or public files.
    Args:
        user_id (UUID): The ID of the user whose files to retrieve.
        db (Session): The database session.

    Returns:
        List[File]: List of files owned by the user or public files.
    """
    # Check if the user exists in the database
    user = db.query(User).filter(User.id == user_id).first()

    if not user:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND,
                            detail="User does not exist")

    # Retrieve files owned by the user or public files
    files = db.query(File).filter(or_(File.owner_id == user_id, File.is_public is True)).all()

    return files


def copy_file(file_id: UUID, user_id: UUID,
              file_path: str, db: Session) -> None:
    """
        Copy a file with the specified ID to a new location.

        Args:
            file_id (UUID): The ID of the file to be copied.
            user_id (UUID): The ID of the user who requested file copy.
            file_path (str): The new path where the file will be copied.
            db (Session): The database session.

        Raises:
            HTTPException: If the source file is not found or if an internal server error occurs.
    """
    # Check if the source file exists in the database
    copyfile = db.query(File).filter(File.id == file_id).first()
    if not copyfile:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND,
                            detail="Source file not found.")
    if copyfile.owner_id != user_id and copyfile.is_public is False:
        raise HTTPException(status_code=status.HTTP_403_FORBIDDEN,
                            detail="Bad access.")

    # Begin a transaction for database operations
    try:
        # Create a copy of the file metadata with a new ID and path
        new_file = copyfile.copy()
        new_file.path = file_path
        new_file.owner_id = user_id
        db.add(new_file)

        # Construct source and destination file paths
        src_file_path = construct_file_path(user_id=copyfile.owner_id, file_id=copyfile.id,
                                            file_path=copyfile.path, file_type=copyfile.mime_type)
        dst_file_path = construct_file_path(user_id=new_file.owner_id, file_id=new_file.id,
                                            file_path=file_path, file_type=new_file.mime_type)

        # Ensure the destination directory exists
        dst_file_path.parent.mkdir(parents=True, exist_ok=True)

        # Copy the file
        shutil.copy(src_file_path, dst_file_path)
    except Exception as e:
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"Failed to copy file: {e}")


def get_file_metadata_by_id(file_id: UUID, user_id: UUID, db: Session) -> Union[File, None]:
    """
    Get file metadata from the database by ID.

    Args:
        file_id (UUID): ID of the file to be searched.
        user_id (UUID): ID of the user who requested metadata of the file.
        db (Session): The database session.

    Returns:
        File: The file metadata if found, None otherwise.
    """
    file_metadata = db.query(File).get(file_id)
    if not file_metadata:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND,
                            detail="File not found.")
    if file_metadata.owner_id != user_id and file_metadata.is_public is False:
        raise HTTPException(status_code=status.HTTP_403_FORBIDDEN,
                            detail="Bad access.")
    return file_metadata


def update_file_metadata(file_id: UUID, user_id: UUID,
                         metadata: FileUpdate, db: Session) -> None:
    """
        Update file metadata in the database.

        Args:
            file_id (UUID): ID of the file to be updated.
            user_id (UUID): ID of the user who requested file update.
            metadata (FileUpdate): Updated metadata.
            db (Session): The database session.
    """
    file = db.query(File).get(file_id)
    if not file:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND,
                            detail="File not found.")
    if file.owner_id != user_id:
        raise HTTPException(status_code=status.HTTP_403_FORBIDDEN,
                            detail="Bad access.")

    # Move file data object to new path if it differs
    if metadata.path != file.path:
        old_path = construct_file_path(user_id=user_id, file_id=file_id,
                                       file_path=file.path, file_type=file.mime_type)
        new_path = construct_file_path(user_id=user_id, file_id=file_id,
                                       file_path=metadata.path, file_type=file.mime_type)

        # Ensure the destination directory exists
        new_path.parent.mkdir(parents=True, exist_ok=True)

        shutil.move(old_path, new_path)

    for key, value in metadata.dict().items():
        setattr(file, key, value)


def delete_file_metadata(file_id: UUID, user_id: UUID, db: Session) -> Path:
    """
        Delete file metadata from the database.

        Args:
            file_id (UUID): ID of the file to be deleted.
            user_id (UUID): ID of the user who requested file removal.
            db (Session): The database session.

        Returns:
            str: Path of file data object to be deleted.
    """
    file = db.query(File).get(file_id)
    if not file:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND,
                            detail="File not found.")
    if file.owner_id != user_id:
        raise HTTPException(status_code=status.HTTP_403_FORBIDDEN,
                            detail="Bad access.")
    file_path = construct_file_path(user_id=file.owner_id, file_id=file.id,
                                    file_path=file.path, file_type=file.mime_type)
    db.delete(file)

    return file_path

# def get_all_public_files(db: Session):
#     query = select(File).where(File.is_public == 1)
#     result = db.execute(query)
#     response = [row.to_json_no_blob() for row in result.scalars()]
#     return response
#
#
# def get_public_file_by_id(db: Session, file_id: int):
#     query = select(File).where(and_(File.is_public == 1, File.id == file_id))
#     result = db.execute(query)
#     response = [row.to_json_no_blob() for row in result.scalars()]
#     return response
