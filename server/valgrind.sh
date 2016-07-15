#!/bin/bash

valgrind --trace-children=no --child-silent-after-fork=yes --leak-check=full --show-leak-kinds=all $@ 
