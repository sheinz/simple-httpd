# Extemely simple http web server

[![Build Status](https://travis-ci.org/sheinz/simple-httpd.svg?branch=master)](https://travis-ci.org/sheinz/simple-httpd)

## Features

* Using plain sockets.
* Handle requests using user callbacks
* Support POST data (file upload)
* Single-threaded, only one request a time.

## Use

Originaly intended as a web server for esp8266 for configuration and firmware
update.

To test it for host system, go to directory test ```make``` and run ```./test```
