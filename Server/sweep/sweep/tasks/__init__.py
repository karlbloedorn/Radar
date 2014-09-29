from celery import Celery


app = Celery('tasks', backend='redis://localhost', broker='redis://localhost/')

import l2

if __name__ == "__main__":
    mirror_base = "http://mesonet-nexrad.agron.iastate.edu/level2/raw/"
    sites = l2.RadarSites(mirror_base)
    sites.update()
