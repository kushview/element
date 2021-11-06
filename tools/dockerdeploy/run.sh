#!/bin/sh
# Runs the kushview/element docker image. By default it starts
# a bash shell.

entry="${1}"
if [ -z "${entry}" ]; then
    entry="/bin/bash"
fi

mkdir -p dist

docker run --rm -it \
    --entrypoint "${entry}" \
    --volume `pwd`/dist:/dist \
    --volume `pwd`:/element \
    --workdir /element \
    -u $(id -u ${USER}):$(id -g ${USER}) \
    kushview/element
