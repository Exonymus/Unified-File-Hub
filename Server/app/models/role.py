from database import Base
from sqlalchemy import Column, Integer, String
from sqlalchemy_utils import UUIDType
from uuid import uuid4


class Role(Base):
    __tablename__ = 'roles'

    id = Column(Integer, primary_key=True, autoincrement=True, nullable=False)
    role_name = Column(String(255), nullable=False)
