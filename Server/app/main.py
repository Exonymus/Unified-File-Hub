from fastapi import FastAPI, Response, UploadFile, File
from services import UserService, FileService
from pydantic import BaseModel

app = FastAPI()
user_service = UserService()
file_service = FileService()

class Data(BaseModel):
    fileData: str
    

#file requests
@app.post('/auth_user')
def auth_user(username, password):
    return user_service.auth_user(username, password)

@app.post('/edit_user')
def edit_user(user_id, email):
    return user_service.update_user(user_id, email)

@app.post('/get_user_info')
def get_user_info(username):
    return user_service.get_user_info(username)



#file requests
@app.post('/get_user_files')
def get_user_files(username):
    return file_service.get_user_files(username)

@app.post('/get_user_files_metadata')
def get_user_files_metadata(username):
    return file_service.get_user_files_metadata(username)

@app.post('/upload_file')
async def upload_file(download_id, filename, author_name, publication_name, theme, publication_date, 
                    description, upload_date, uploader_name, doc_type, is_public, folder_path, input_data: UploadFile = File(...)):
    return file_service.upload_file(download_id, filename, author_name, publication_name, theme, publication_date, 
                    description, upload_date, uploader_name, doc_type, is_public, input_data, folder_path)

@app.post('/copy_file')
def copy_file(file_id, folder_path, username):
    return file_service.copy_file(file_id, folder_path, username)

@app.post('/delete_file')
def delete_file(file_id):
    return file_service.delete_file(file_id)

@app.post('/update_file')
def update_file(file_id, author_name, publication_name, theme, publication_date, description, is_public, folder_path):
    return file_service.update_file(file_id, author_name, publication_name, theme, publication_date, description, is_public, folder_path)

@app.post('/get_public_file_by_id')
def get_public_file_by_id(file_id):
    return file_service.get_public_file_by_id(file_id)

@app.post('/get_all_public_files')
def get_all_public_files():
    return file_service.get_all_public_files()

@app.post("/upload_file_data")
async def upload_file_data(file: UploadFile = File(...)):
    return {"filename": file.filename}