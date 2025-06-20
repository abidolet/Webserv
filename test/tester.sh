#!/bin/bash

exec ../webserv ../conf/default.conf &
exec siege -c 2 -r 10 -b -t 1s --delay=0 http://127.0.0.1:8080
