from sqlalchemy import Column, Integer, String, Boolean
from sqlalchemy.ext.declarative import declarative_base

Base = declarative_base()

class Users(Base):
    __tablename__ = 'users'

    id = Column(Integer, primary_key=True)
    username = Column(String)
    email = Column(String)
    password = Column(String)
    on_active = Column(Boolean)
    is_banned = Column(Boolean)
    is_actual = Column(Boolean)
    role = Column(Integer)
    
    def to_json(self):
        return {"id": self.id, "username": self.username, "email": self.email,
                "on_active": self.on_active, "is_banned": self.is_banned, "is_actual": self.is_actual,
                "role": self.role}