#!/bin/bash
export LD_LIBRARY_PATH="`pwd`/build/lib" 
./build/bin/test-element $@
exit $?
