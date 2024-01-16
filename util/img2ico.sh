#!/bin/bash
# converts an image to a Windows ICO file. Required Imagemagick convert.

convert -background transparent \
    "${1}" -define icon:auto-resize=16,24,32,48,64,72,96,128,256 \
    "${2}"
