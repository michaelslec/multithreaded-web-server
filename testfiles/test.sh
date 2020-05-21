#!/bin/bash

# curl -T largetxt http://localhost:8080/largeput &
# curl http://localhost:8080/largeput &
curl -T smalltxt http://localhost:8080/smallput2 > out &
curl -T largetxt http://localhost:8080/largeput > out &
curl -T smalltxt http://localhost:8080/smallput > out &
# curl -T smallbin http://localhost:8080/bin
# curl -I http://localhost:8080/bin &
# curl http://localhost:8080/largeput &
# curl http://localhost:8080/smallput &
