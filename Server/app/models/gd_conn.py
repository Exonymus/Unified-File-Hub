from database import Base
from sqlalchemy import Column, String, ForeignKey
from sqlalchemy.orm import relationship
from sqlalchemy_utils import UUIDType
from uuid import uuid4


class GDConn(Base):
    __tablename__ = 'gd_conns'

    id = Column(UUIDType(binary=False), primary_key=True, default=uuid4)
    user_id = Column(UUIDType(binary=False), ForeignKey("users.id"), nullable=False)
    access_key = Column(String(255), nullable=False)
    refresh_key = Column(String(255), nullable=False)

    user = relationship("User", back_populates="connections_G")
