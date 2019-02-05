#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pigpio

class FlowSensor:

    def __init__(self, pi, pin):
        self._pi = pi
        self._pin = pin
        pi.set_mode(pin, pigpio.INPUT)

        self._prev_tally = 0
        self._cb = pi.callback(pin, pigpio.RISING_EDGE)

    def close(self):
        self._cb.clear()

    def reset(self):
        self._prev_tally = self._cb.tally()

    def _count_passed(self):
        tally = self._cb.tally()
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
