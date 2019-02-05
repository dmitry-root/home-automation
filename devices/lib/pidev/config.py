#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import json

def prefix():
    path = os.getenv("DEEPHOME_PATH")
    if path:
        return path.rstrip('/')
    path = os.path.dirname( os.path.abspath(__file__) )
    return path.replace('/devices/lib/pidev', '')

def read(name):
    path = prefix() + "/etc/" + name
    with open(path) as file:
        return json.load(file)

if __name__ == '__main__':
    print("Prefix: %s" % prefix())
