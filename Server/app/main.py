from fastapi import FastAPI, APIRouter
from routers.user import router as user_router
from routers.file import router as file_router

router = APIRouter()
router.include_router(
    user_router,
    prefix='/users',
    tags=['users']
)
router.include_router(
    file_router,
    prefix='/files',
    tags=['files']
)

app = FastAPI()
app.include_router(router)
