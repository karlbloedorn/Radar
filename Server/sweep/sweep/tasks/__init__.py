from celery import Celery


app = Celery('tasks', backend='redis://localhost', broker='redis://localhost/')

import l2

