import re
import grequests
import datetime

import gevent
import socket
from celery import current_app
from celery import chain
from celery.utils.log import get_task_logger
import boto
import os

logger = get_task_logger(__name__)

class RadarProduct(object):
    filename_regex = re.compile("^(?P<size>\d+) (?P<filename>(?P<callsign>[A-Z]{4})_(?P<year>\d{4})(?P<month>\d{2})(?P<day>\d{2})_(?P<hour>\d{2})(?P<minute>\d{2}))$")
    def __init__(self, filename):
        match = self.filename_regex.search(filename)
        fields = match.groupdict()
        for key in ('year', 'month', 'day', 'hour', 'minute', 'size'):
            fields[key] = int(fields[key])
        self.filename = fields['filename']
        self.size = fields['size']
        self.dt = datetime.datetime(fields['year'], fields['month'], fields['day'], hour=fields['hour'], minute=fields['minute'])
        self.callsign = fields['callsign']

    def __repr__(self):
        return 'RadarProduct {} - {} {}'.format(self.callsign, self.dt.isoformat(), self.size)


    def timedelta(self, ref=None):
        if ref is None:
            ref = datetime.datetime.utcnow()
        return self.dt - ref

    def url(self, base):
        return base + self.callsign + '/' + self.filename

    def request(self, base):
        return grequests.get(self.url(base))

mirror_base = "http://mesonet-nexrad.agron.iastate.edu/level2/raw/"

class RadarSites(object):
    def __init__(self, base_url):
        self.base_url  = base_url
        self.callsigns = set()
        self.list_file = None
        self._pull_config()

    def _pull_config(self):
        config_file = grequests.get(mirror_base + "config.cfg").send()
        for line in config_file.text.splitlines():
            match = re.match('^[Ss]ite\: ([A-Z]{4})$', line)
            if match is not None:
                self.callsigns.add(match.groups()[0])
            else:
                match = re.match('^ListFile\: (.*)$', line)
                if match is not None:
                    self.list_file = match.groups()[0]

    def update(self, callsigns=None):
        if callsigns is None:
            callsigns = self.callsigns
        else:
            callsigns = self.callsigns & callsigns
        results = {}
        for callsign in callsigns:
            url = self.base_url + callsign + '/' + self.list_file
            results[callsign] = chain(fetch_dirlist.s(url, callsign), process.s(callsign))()
        return results

class Radar(object):
    def __init__(self, callsign):
        pass

    @property
    def last_updated(self):
        return datetime.datetime.utcnow() - datetime.timedelta(seconds=450)
        pass

    @last_updated.setter
    def last_updated_set(self, value):
        pass

    def process(self, product, product_url):
        s3 = boto.s3.connect_to_region(region_name='us-west-2')
        bucket = s3.get_bucket('a9g4c-l2')
        key_name = '{}_{}.bin'.format(product.callsign, product.dt.isoformat())
        path = product.callsign
        full_key_name = os.path.join(path, key_name)
        k = bucket.new_key(full_key_name)
        k.metadata['Content-Encoding'] = 'gzip'

        process_socket = gevent.socket.create_connection(("127.0.0.1", 6436))
        product_request = grequests.get(product_url).send(stream=True)
        for chunk in product_request.iter_content(chunk_size=1024 * 8):
            process_socket.sendall(chunk)

        radar_data = ''
        while True:
            chunk = process_socket.recv(2048)
            if not chunk:
                break
            radar_data += chunk

        logger.info("Processed file contents")
        k.set_contents_from_string(radar_data)
        return k.generate_url(1800)

    def fetch_dirlist(self, url):
        dir_list = grequests.get(url).send()
        product_lines = [line for line in dir_list.text.splitlines() if line]
        target_product = product_lines[-2:-1][0]
        product = RadarProduct(target_product)
        if True or product.timedelta(self.last_updated).total_seconds() > 60:
            return target_product
        return None


@current_app.task
def fetch_dirlist(url, callsign):
    target_product = Radar(callsign).fetch_dirlist(url)
    if target_product is None:
        logger.info("No new product for %s", callsign)
    return target_product

@current_app.task
def process(target_product, callsign):
    logger.info("Begin processing for %s with %s", callsign, target_product)
    if target_product is not None:
        radar = Radar(callsign)
        product = RadarProduct(target_product)
        url = product.url(mirror_base)
        return radar.process(product, url)
    else:
        logger.info("No new product, skipping!")

if __name__ == "__main__":
    mirror_base = "http://mesonet-nexrad.agron.iastate.edu/level2/raw/"
    sites = RadarSites(mirror_base)
    print sites.update(set(['KMPX']))['KMPX'].get()
