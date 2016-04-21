#!/bin/bash

g++ --std=c++11 -o twilio-cnam twilio-cnam.cpp `curl-config --libs` -lsqlite3
