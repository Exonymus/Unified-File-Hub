from database import Base, engine
from .files import Files
from .roles import Roles
from .users import Users


Base.metadata.create_all(bind=engine)
