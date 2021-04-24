#!/bin/bash
export LD_LIBRARY_PATH="`pwd`/build/lib" 
./build/bin/test_juce $@
exit $?
