#!/bin/bash

../webserv ../conf/action.conf &
siege -c 20 -r 100 -b -t 30m --delay=0 http://127.0.0.1:8080 > log.txt

sleep 5
cat log.txt
