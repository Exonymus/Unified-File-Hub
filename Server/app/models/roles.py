from database import Base
from sqlalchemy import Column, Integer, String
from sqlalchemy_utils import UUIDType
from uuid import uuid4


class Roles(Base):
    __tablename__ = 'roles'

    id = Column(UUIDType(binary=False), primary_key=True, default=uuid4)
    role_name = Column(String(255), nullable=False)
