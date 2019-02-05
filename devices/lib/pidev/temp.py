#!/usr/bin/env python3
# -*- coding: utf-8 -*-

class TempSensor:

    def __init__(self, dev_id):
        self._dev_id = dev_id

    def read(self):
        dev_file = '/sys/bus/w1/devices/' + self._dev_id + '/w1_slave'
        lines = []
        with open(dev_file) as file:
            lines = file.readlines()
        if not lines or len(lines) < 2:
            return None
        if lines[0].strip()[-3:] != 'YES':
            return None

        eq_pos = lines[1].find('t=')
        if eq_pos < 0:
            return None
        temp = lines[1][eq_pos+2:]
        return float(temp)/1000
