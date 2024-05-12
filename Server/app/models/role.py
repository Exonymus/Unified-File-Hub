from database import Base
from sqlalchemy import Column, String
from sqlalchemy.orm import relationship
from sqlalchemy_utils import UUIDType
from uuid import uuid4


class Role(Base):
    __tablename__ = 'roles'

    id = Column(UUIDType(binary=False), primary_key=True, default=uuid4)
    role_name = Column(String(255), nullable=False)

    users = relationship("User", back_populates="role")
