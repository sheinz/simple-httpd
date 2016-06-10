#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import requests


def main():
    payload = {'param_1': ('', 'value_1'),
               'param_2': ('', 'value_2'),
               'param_3': ('', 'value_3'),
               'firmware': open('/tmp/test.data', 'rb')}

    r = requests.post('http://localhost:8080/upload', files=payload)
    if r.status_code == 200:
        exit(0)
    else:
        exit(1)

if __name__ == "__main__":
    main()
