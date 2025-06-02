#!/bin/bash

# Test interpretation
echo "Testing basic compilation..."
./myco.exe test.myco

# Test executable building
echo "Testing executable building..."
./myco.exe test.myco --build
./test.exe
