#!/bin/sh
entry="${1}"
if [ -z "${entry}" ]; then
    entry="/bin/bash"
fi

docker run --rm -it \
    --entrypoint "${entry}" \
    --volume `pwd`:/element \
    --workdir /element \
    -u $(id -u ${USER}):$(id -g ${USER}) \
    kushview/element
