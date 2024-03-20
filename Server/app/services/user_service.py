from controllers import DatabaseController

class UserService:
    database_controller = DatabaseController()
    
    def get_user_info(self, username):
        response = self.database_controller.get_user_info(username)
        response_json = {}
        counter = 0
        for item in response:
            response_json[f"{counter}"] = response[counter]
            counter += 1
        return {"data": response_json}
    
    def update_user(self, user_id, email):
        self.database_controller.update_user(user_id, email)
        return {"data": 0}
    
    def auth_user(self, username, password):
        if self.database_controller.auth_user(username, password):
            return {"is_auth": True}
        return {"is_auth": False}