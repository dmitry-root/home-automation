#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
from http.server import HTTPServer, BaseHTTPRequestHandler
import logging
import json
import pigpio
import threading
import time

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../lib'))
from pidev.monitor  import SensorsMonitor
from pidev.periodic import PeriodicTimer
import pidev.config


class PeriodicMonitor:
    _pers_interval = 60.0 * 60.0    # 1 hour

    def __init__(self, temp_file, pers_file, interval, config):
        self.temp_file = temp_file
        self.pers_file = pers_file
        self.interval = interval
        self._load()
        self._save_file(self.temp_file)
        self._timer = PeriodicTimer(self.interval, self._update)
        self._monitor = SensorsMonitor(pigpio.pi(), config)
        self._lock = threading.Lock()

    def start(self):
        with self._lock:
            self._monitor.start()
            self._timer.start()
            self._time = time.clock_gettime(time.CLOCK_MONOTONIC)
            self._pers_time = self._time

    def stop(self):
        with self._lock:
            self._monitor.stop()
            self._timer.stop()

    def _load(self):
        if not self._load_file(self.temp_file):
            self._load_file(self.pers_file)
        self._data['temp'] = {}
        self._data['flow'] = {}
        self._data.setdefault('consumption', {})

    def _load_file(self, file):
        try:
            with open(file) as fd:
                self._data = json.load(fd)
        except:
            return False
        return True

    def _save_file(self, file):
        tmp_file = file + '.tmp'
        try:
            with open(tmp_file, 'w') as fd:
                json.dump(self._data, fd, sort_keys=True, indent=4)
            os.rename(tmp_file, file)
        except:
            logging.error("failed to save data into file: '%s', error: '%s'" % (file, sys.exc_info()[0]))
        finally:
            try:
                os.unlink(tmp_file)
            except:
                pass

    def _update(self):
        with self._lock:
            self._do_update()

    def _do_update(self):
        now = time.clock_gettime(time.CLOCK_MONOTONIC)
        seconds_passed = now - self._time
        self._time = now

        data = self._monitor.collect(seconds_passed)
        self.data['temp'] = data.temp
        self.data['flow'] = data.flow
        for key, value in data.consumption.items():
            self._data['consumption'].setdefault(key, 0.0)
            self._data['consumption'][key] += value

        self._save_file(self.temp_file)
        if now > self._pers_time + PeriodicMonitor._pers_interval:
            self._pers_time = now
            self._save_file(self.pers_file)


if __name__ == '__main__':
    config = pidev.config.read('websensor.json')
    prefix = pidev.config.prefix()

    # Default configuration
    port = 8000
    interval = 30.0
    temp_dir = '/tmp/websensor'
    pers_dir = '/var/www/websensor'
    file = 'sensordata.json'

    if 'websensor' in config:
        wconfig = config['websensor']
        try:
            port = int(wconfig.get('port', port))
            interval = float(wconfig.get('interval', interval))
            temp_dir = wconfig.get('temporary_directory', temp_dir)
            pers_dir = wconfig.get('persistent_directory', pers_dir)
        except:
            logging.error("error parsing websensor config: %s" % sys.exc_info()[0])

    os.chdir(temp_dir)
    temp_file = temp_dir + '/' + file
    pers_file = pers_dir + '/' + file
    monitor = PeriodicMonitor(temp_file, pers_file, interval, config)
    server = HTTPServer(('', port), SimpleHTTPRequestHandler)

    logging.info("starting websensor at port: %d" % port)
    monitor.start()
    server.serve_forever()

    monitor.stop()
    logging.info("websensor stopped")

