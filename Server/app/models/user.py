from database import Base
from sqlalchemy import Column, Integer, String, Boolean, ForeignKey, DateTime
from sqlalchemy.orm import relationship
from sqlalchemy_utils import UUIDType
from uuid import uuid4


class User(Base):
    __tablename__ = 'users'

    id = Column(UUIDType(binary=False), primary_key=True, default=uuid4)
    role_id = Column(UUIDType(binary=False), ForeignKey("roles.id"), nullable=False)
    username = Column(String(255), nullable=False, unique=True)
    email = Column(String(255), nullable=False, unique=True)
    secret_num = Column(Integer, nullable=False)
    secret_answer = Column(String(255), nullable=False)
    hashed_password = Column(String(255), nullable=False)
    reg_date = Column(DateTime(), nullable=False)
    is_banned = Column(Boolean, nullable=False)

    role = relationship("Role", back_populates="users")
    files = relationship("File", back_populates="owner")

    connections_G = relationship("GDConn", back_populates="user")
    connections_O = relationship("ODConn", back_populates="user")
    connections_F = relationship("FTPConn", back_populates="user")
