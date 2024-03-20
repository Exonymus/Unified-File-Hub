from controllers import DatabaseController

class FileService:
    database_controller = DatabaseController()
    
    def get_user_files(self, username):
        response = self.database_controller.get_user_files(username)
        response_json = {}
        counter = 0
        for item in response:
            response_json[f"{counter}"] = response[counter]
            counter += 1
        return {"data": response_json}
    
    def get_user_files_metadata(self, username):
        response = self.database_controller.get_user_files_metadata(username)
        response_json = {}
        counter = 0
        for item in response:
            response_json[f"{counter}"] = response[counter]
            counter += 1
        return {"data": response_json}
    
    def delete_file(self, file_id):
        self.database_controller.delete_file(file_id)
        return {"data": 0}
    
    def update_file(self, file_id, author_name, publication_name, theme, publication_date, description, is_public, folder_path):
        self.database_controller.update_file(file_id, author_name, publication_name, theme, publication_date, description, is_public, folder_path)
        return {"data": 0}
    
    def upload_file(self, download_id, filename, author_name, publication_name, theme, publication_date, 
                    description, upload_date, uploader_name, doc_type, is_public, input_data, folder_path):
        self.database_controller.create_file(download_id, filename, author_name, publication_name, theme, publication_date, 
                    description, upload_date, uploader_name, doc_type, is_public, input_data, folder_path)
        return {"data": 0}
    
    def copy_file(self, file_id, folder_path, username):
        self.database_controller.copy_file(file_id, folder_path, username)
        
    def get_public_file_by_id(self, file_id):
        return {"data": self.database_controller.get_public_file_by_id(file_id)}
    
    def get_all_public_files(self):
        response = self.database_controller.get_all_public_files()
        response_json = {}
        counter = 0
        for item in response:
            response_json[f"{counter}"] = response[counter]
            counter += 1
        return {"data": response_json}