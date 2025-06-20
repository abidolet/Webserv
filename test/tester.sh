#!/bin/bash

../webserv ../conf/action.conf & > /dev/null 2>&1 &
siege -c 4 -r 10 -b -t 10s --delay=0 http://127.0.0.1:8080 > log.txt

sleep 5
cat log.txt
