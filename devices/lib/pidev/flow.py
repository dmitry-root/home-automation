#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import RPi.GPIO as GPIO
import threading

class FlowSensor:

    def __init__(self, pin):
        self._pin = pin
        GPIO.setup(pin, GPIO.IN)
        self._lock = threading.Lock()
        self._prev_tally = 0
        self._tally_counter = 0
        GPIO.add_event_detect(pin, GPIO.RISING)
        GPIO.add_event_callback(pin, self._callback)

    def close(self):
        GPIO.remove_event_detect(self._pin)

    def reset(self):
        self._prev_tally = self._tally()

    def _callback(self, pin):
        with self._lock:
            self._tally_counter += 1

    def _tally(self):
        with self._lock:
            return self._tally_counter

    def _count_passed(self):
        tally = self._tally()
        count = tally - self._prev_tally
        self._prev_tally = tally
        return count

    def get_litres(self, seconds_passed):
        count = self._count_passed()
        if count == 0:
            return 0.0
        return (count + 8*seconds_passed) / 360

    def get_litres_per_min(self, seconds_passed):
        count = self._count_passed()
        if count == 0:
            return 0.0
        return (count/seconds_passed + 8) / 6
