
from os import  environ
from os.path import  abspath, dirname 
from dotenv import load_dotenv

load_dotenv()  # load environment variables from .env if it exists.
basedir = abspath(dirname(__file__))


def _to_bool(value, default=False):
    if value is None:
        return default
    return str(value).strip().lower() in {"1", "true", "yes", "on"}


def _to_int(value, default):
    try:
        return int(value)
    except (TypeError, ValueError):
        return default

class Config(object):
    """Base Config Object"""
    

    FLASK_DEBUG                             = _to_bool(environ.get('DEBUG', environ.get('FLASK_DEBUG', 'False')))
    SECRET_KEY                              = environ.get('SECRET_KEY', 'Som3$ec5etK*y')
    UPLOADS_FOLDER                          = environ.get('UPLOADS_FOLDER', 'app/static/uploads') 
    IMAGE_FOLDER                            = environ.get('IMAGE_FOLDER', 'app/static/images') 

    ENV                                     = environ.get('FLASK_DEBUG') 
    FLASK_RUN_PORT                          = _to_int(environ.get('FLASK_RUN_PORT'), 8080) 
    FLASK_RUN_HOST                          = environ.get('FLASK_RUN_HOST', '127.0.0.1') 

    # MONGODB VARIABLES
    DB_USERNAME                             = environ.get('DB_USERNAME') 
    DB_PASSWORD                             = environ.get('DB_PASSWORD') 
    DB_SERVER                               = environ.get('DB_SERVER', 'localhost') 
    DB_PORT                                 = _to_int(environ.get('DB_PORT'), 27017) 

    PROPAGATE_EXCEPTIONS                    = False
 
 
