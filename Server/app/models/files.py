from sqlalchemy import Column, Integer, String, Boolean, BLOB
from sqlalchemy.ext.declarative import declarative_base

Base = declarative_base()

class Files(Base):
    __tablename__ = 'files'

    id = Column(Integer, primary_key=True)
    filename = Column(String)
    download_id = Column(String)
    author_name = Column(String)
    publication_name = Column(String)
    theme = Column(String)
    publication_date = Column(String)
    description = Column(String)
    upload_date = Column(String)
    uploader_name = Column(String)
    folder_path = Column(String)
    doc_type = Column(String)
    is_public = Column(Boolean)
    data = Column(BLOB)
    
    def to_json(self):
        return {"id": self.id, "filename": self.filename, "download_id": self.download_id,
                "author_name": self.author_name, "publication_name": self.publication_name, "theme": self.theme,
                "publication_date": self.publication_date, "description": self.description, "upload_date": self.upload_date,
                "uploader_name": self.uploader_name, "folder_path": self.folder_path, "doc_type": self.doc_type,
                "is_public": self.is_public, "data": str(self.data)}
        
    def to_json_no_blob(self):
        return {"id": self.id, "filename": self.filename, "download_id": self.download_id,
                "author_name": self.author_name, "publication_name": self.publication_name, "theme": self.theme,
                "publication_date": self.publication_date, "description": self.description, "upload_date": self.upload_date,
                "uploader_name": self.uploader_name, "folder_path": self.folder_path, "doc_type": self.doc_type,
                "is_public": self.is_public, "file_size": len(self.data)}
