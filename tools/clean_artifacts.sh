#!/bin/bash
find build -type f -name "*.a" -delete
find build -type f -name "*.dll" -delete
find build -type f -name "*.exe" -delete
find build -type f -name "*.so"  -delete
find build -type f -name "element*"  -delete
