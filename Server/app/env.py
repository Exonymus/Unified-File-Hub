import os

APP_ENV = os.environ.get('APP_ENV')

DB_USER = os.environ.get('MYSQL_USER')
DB_PASSWORD = os.environ.get('MYSQL_PASSWORD')
DB_HOST = os.environ.get('MYSQL_HOST')
DB_NAME = os.environ.get('MYSQL_DATABASE')

JWT_SECRET_KEY = os.environ.get('SECRET_KEY')
JWT_ALGORITHM = os.environ.get('ALGORITHM')
JWT_EXPIRE = os.environ.get('ACCESS_TOKEN_EXPIRE_MINUTES')
