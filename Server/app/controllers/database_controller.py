from sqlalchemy import create_engine, MetaData, Table, select, Engine, or_, and_
from sqlalchemy.orm import sessionmaker
import bcrypt
import uuid

from models import Files, Users

class DatabaseController:
    engine: Engine

    def __init__(self):
        self.engine = create_engine('mariadb+mariadbconnector://desuuser:S0siteXui!1User@174.138.14.230:3306/desurep')
        
    def auth_user(self, username, password):
        SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=self.engine)
        session = SessionLocal()
        
        user = session.query(Users).filter(Users.username == username).first()
        if not user:
            session.close()
            return False
        if not bcrypt.checkpw(password.encode('utf-8'), user.password.encode('utf-8')):
            session.close()
            return False
        
        session.close()
        return True

    def get_user_info(self, username):
        SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=self.engine)
        session = SessionLocal()
        query = select(Users).where(Users.username == username)
        result = session.execute(query)
        response = [row.to_json() for row in result.scalars()]
        session.close()
        return response
    
    def update_user(self, user_id, email):
        SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=self.engine)
        session = SessionLocal()
        user = session.query(Users).filter(Users.id == user_id).first()
        setattr(user, "email", email)
        session.commit()
        session.close()
        return 0
    
    def create_file(self, download_id, filename, author_name, publication_name, theme, 
                    publication_date, description, upload_date, uploader_name, doc_type, 
                    is_public, input_data, folder_path):
        SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=self.engine)
        session = SessionLocal()
        file = Files(download_id=download_id, filename=filename, author_name=author_name, publication_name=publication_name,
                     theme=theme, publication_date=publication_date, description=description, upload_date=upload_date, uploader_name=uploader_name,
                     doc_type=doc_type, is_public=bool(int(is_public)), data=input_data.file.read(), folder_path=folder_path)
        session.add(file)
        session.commit()
        session.close()
        return 0
    
    def copy_file(self, file_id, folder_path, username):
        SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=self.engine)
        session = SessionLocal()
        
        file = session.query(Files).filter(Files.id == file_id).first()
        copy_file=Files(filename=str(uuid.uuid4()) + file.publication_name, download_id=str(uuid.uuid4()), author_name=file.author_name, publication_name=file.publication_name,
                        theme=file.theme, publication_date=file.publication_date, description=file.description, upload_date=file.upload_date,
                        uploader_name=username, doc_type=file.doc_type, is_public=0, data=file.data, folder_path=folder_path)
        
        session.add(copy_file)
        session.commit()
        session.close()
    
    def get_user_files(self, username):
        SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=self.engine)
        session = SessionLocal()
        query = select(Files).where(or_(Files.uploader_name == username, Files.is_public == 1))
        result = session.execute(query)
        response = [row.to_json() for row in result.scalars()]
        session.close()
        return response
    
    def get_user_files_metadata(self, username):
        SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=self.engine)
        session = SessionLocal()
        query = select(Files).where(or_(Files.uploader_name == username, Files.is_public == 1))
        result = session.execute(query)
        response = [row.to_json_no_blob() for row in result.scalars()]
        session.close()
        return response
    
    def delete_file(self, file_id):
        SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=self.engine)
        session = SessionLocal()
        file = session.query(Files).filter(Files.id == file_id).first()
        session.delete(file)
        session.commit()
        session.close()
        return 0
    
    def update_file(self, file_id, author_name, publication_name, theme, publication_date, description, is_public, folder_path):
        SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=self.engine)
        session = SessionLocal()
        file = session.query(Files).filter(Files.id == file_id).first()
        setattr(file, "author_name", author_name)
        setattr(file, "publication_name", publication_name)
        setattr(file, "theme", theme)
        setattr(file, "publication_date", publication_date)
        setattr(file, "description", description)
        setattr(file, "is_public", bool(int(is_public)))
        setattr(file, "folder_path", folder_path)
        session.commit()
        session.close()
        return 0
    
    def get_all_public_files(self):
        SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=self.engine)
        session = SessionLocal()
        query = select(Files).where(Files.is_public == 1)
        result = session.execute(query)
        response = [row.to_json_no_blob() for row in result.scalars()]
        session.close()
        return response
    
    def get_public_file_by_id(self, file_id):
        SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=self.engine)
        session = SessionLocal()
        query = select(Files).where(and_(Files.is_public == 1, Files.id == file_id))
        result = session.execute(query)
        response = [row.to_json_no_blob() for row in result.scalars()]
        session.close()
        return response