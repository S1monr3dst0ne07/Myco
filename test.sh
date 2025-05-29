#!/bin/bash

# Test basic compilation
echo "Testing basic compilation..."
./bin/myco test.myco

# Test executable building
echo "Testing executable building..."
./bin/myco --build test.myco
./test
