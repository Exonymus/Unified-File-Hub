from database import Base
from sqlalchemy import Column, String, ForeignKey
from sqlalchemy.orm import relationship
from sqlalchemy_utils import UUIDType
from uuid import uuid4


class FTPConn(Base):
    __tablename__ = 'ftp_cons'

    id = Column(UUIDType(binary=False), primary_key=True, default=uuid4)
    user_id = Column(UUIDType(binary=False), ForeignKey("users.id"), nullable=False)
    name = Column(String(255), nullable=False)
    ftp_user = Column(String(255), nullable=False)
    ftp_pass = Column(String(255), nullable=False)
    ftp_ip = Column(String(255), nullable=False)

    user = relationship("User", back_populates="connections_F")

