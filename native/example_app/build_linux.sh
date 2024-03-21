#!/usr/bin/env sh

clang -g -lm -lGL -lX11 -lXcursor -lXi \
    src/example_app.c src/ufbx.c src/test.c \
    -o example 
