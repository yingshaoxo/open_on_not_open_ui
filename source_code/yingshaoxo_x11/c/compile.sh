#!/bin/bash

gcc -Wall -O2 -o window.run a_window.c -lX11 -pthread
