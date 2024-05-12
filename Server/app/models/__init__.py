from database import Base, engine, SessionLocal
from .file import File
from .role import Role
from .user import User
from .ftp_conn import FTPConn
from .gd_conn import GDConn
from .od_conn import ODConn

Base.metadata.create_all(bind=engine)


def init_roles():
    db = SessionLocal()
    try:
        # Check if roles already exist
        existing_roles = db.query(Role).all()
        if not existing_roles:
            # Add 'user' role
            user_role = Role(role_name="user")
            db.add(user_role)

            # Add 'admin' role
            admin_role = Role(role_name="admin")
            db.add(admin_role)

            db.commit()
    finally:
        db.close()


# Initialize roles on startup
init_roles()
