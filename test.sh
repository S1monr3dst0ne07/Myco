#!/bin/bash

# Test interpretation
echo "Testing basic compilation..."
./myco test.myco

# Test executable building
echo "Testing executable building..."
./myco test.myco --build
./test
