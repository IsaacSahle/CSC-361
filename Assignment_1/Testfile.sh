#!/bin/bash

(echo -e -n "GET / HTTP/1.0\r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8000 &)
sleep 1
(echo -e -n "GET /noFile HTTP/1.0\r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8000 &)
sleep 1
(echo -e -n "GET / HTTP/1.1\r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8000 &)
sleep 1
(echo -e -n "GET badrequest HTTP/1.0\r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8000 &)
sleep 1
(echo -e -n "GET /my_file HTTP/1.0\r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8000 &) 
