from database import Base
from sqlalchemy import Column, Integer, String, Boolean
from sqlalchemy_utils import UUIDType
from uuid import uuid4


class Users(Base):
    __tablename__ = 'users'

    id = Column(UUIDType(binary=False), primary_key=True, default=uuid4)
    username = Column(String(255), nullable=False)
    email = Column(String(255), nullable=False)
    password = Column(String(255), nullable=False)
    on_active = Column(Boolean, nullable=False)
    is_banned = Column(Boolean, nullable=False)
    is_actual = Column(Boolean, nullable=False)
    role = Column(UUIDType(binary=False), default=uuid4)

    def to_json(self):
        return {"id": self.id, "username": self.username, "email": self.email,
                "on_active": self.on_active, "is_banned": self.is_banned, "is_actual": self.is_actual,
                "role": self.role}
