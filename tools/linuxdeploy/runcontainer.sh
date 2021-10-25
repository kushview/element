#!/bin/sh
docker run --rm -it \
    --entrypoint /bin/bash \
    --volume `pwd`:/element \
    --workdir /element \
    -u $(id -u ${USER}):$(id -g ${USER}) \
    kushview/element
