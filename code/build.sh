#!/bin/sh

gcc -O3 flip_it.c -lraylib -lm -ldl -lpthread -o ../build/flip_it
