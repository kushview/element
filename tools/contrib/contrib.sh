#!/bin/bash
set -x
go build
./contrib > ../../data/developers.txt
