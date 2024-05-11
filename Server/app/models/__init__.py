from database import Base, engine
from .file import File
from .role import Role
from .user import User


Base.metadata.create_all(bind=engine)
