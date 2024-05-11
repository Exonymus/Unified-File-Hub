from database import Base
from sqlalchemy import Column, Integer, String, Boolean
from sqlalchemy_utils import UUIDType
from uuid import uuid4


class User(Base):
    __tablename__ = 'users'

    id = Column(Integer, primary_key=True, autoincrement=True, nullable=False)
    username = Column(String(255), nullable=False)
    email = Column(String(255), nullable=False)
    password = Column(String(255), nullable=False)
    on_active = Column(Boolean, nullable=False)
    is_banned = Column(Boolean, nullable=False)
    is_actual = Column(Boolean, nullable=False)
    role = Column(Integer, nullable=False)

    def to_json(self):
        return {"id": self.id, "username": self.username, "email": self.email,
                "on_active": self.on_active, "is_banned": self.is_banned, "is_actual": self.is_actual,
                "role": self.role}
