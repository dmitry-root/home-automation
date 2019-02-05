#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import config
import flow
import temp
import threading
import time
import logging
import statistics
import pigpio

class SensorsMonitor:

    _temp_timeout = 0.5
    _temp_max = 100

    def __init__(self, pi, config):
        self._temp = {}
        self._flow = {}
        self._temp_results = {}
        if (not config["sensors"]) or (not isinstance(config["sensors"], list)):
            return
        self._fill(pi, config["sensors"])

        self._lock = threading.Lock()
        self._running = False

    def __del__(self):
        self.stop()

    def __enter__(self):
        self.start()

    def __exit__(self):
        self.stop()

    def start(self):
        with self._lock:
            if self._running:
                return
            self._running = True
            logging.info("starting temp monitoring thread")
            self._thread = threading.Thread(target = self._run)
            self._thread.start()

    def stop(self):
        with self._lock:
            if not self._running:
                return
            self._running = False
        self._thread.join()
        logging.info("temp monitoring thread stopped")
        del self._thread

    def collect(self, seconds_passed):
        """Expected to be called periodically by external timer"""

        class Result:
            def __init__(self):
                self.temp = {}
                self.flow = {}

        result = Result()

        # 1. Fill in the flow measurements
        for name, sensor in self._flow.items():
            result.flow[name] = sensor.get_litres(seconds_passed)

        # 2. Fill in the temperature measurements
        with self._lock:
            for name in self._temp_results:
                if not self._temp_results[name]:
                    continue
                result.temp[name] = statistics.mean(self._temp_results[name])
                self._temp_results.clear()

        return result

    def _fill(self, pi, sensors):
        index = 1
        for item in sensors:
            if not isinstance(item, dict):
                continue
            name = item['name']
            if not name:
                name = ('sensor%02d' % (index++))
            type = item['type']
            if type == 'temp':
                self._fill_temp(name, item)
            elif type == 'flow':
                self._fill_flow(name, pi, item)

    def _fill_temp(self, name, item):
        device_id = item['device_id']
        if not device_id:
            logging.warning("device_id not specified for temp sensor '%s', ignored" % name)
            return
        if name in self._temp:
            logging.warning("duplicate name for temp sensor: '%s', ignored" % name)
            return
        logging.info("adding temp sensor name: '%s', device_id: '%s'" % (name, device_id))
        self._temp[name] = temp.TempSensor(device_id)
        self._temp_results[name] = []

    def _fill_flow(self, name, pi, item):
        pin = None
        try:
            pin = int(item['pin'])
        except (KeyError, ValueError):
            logging.warning("could not find correct pin number for flow sensor: '%s', ignored" % name)
            return
        if name in self._flow:
            logging.warning("duplicate name for flow sensor: '%s', ignored" % name)
            return
        logging.info("adding flow sensor name: '%s', pin: %d" % (name, pin))
        self._flow[name] = flow.FlowSensor(pi, pin)

    def _run(self):
        while True:
            for name, sensor in self._temp.items():
                with self._lock:
                    if not self._running:
                        return
                temp = sensor.read()
                with self._lock:
                    if not self._running:
                        return
                    if temp != None:
                        self._temp_results[name].append(temp)
                time.sleep(SensorsMonitor._temp_timeout)
