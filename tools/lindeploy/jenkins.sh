#!/bin/bash

set -e
docker build -t kushview/element .
docker run -i --rm \
    -e WAF_BUILD_OPTIONS="-j4" \
    --entrypoint "tools/lindeploy/entrypoint.sh" \
    --volume `pwd`/dist:/dist \
    --volume `pwd`:/project \
    --volume /opt/SDKs:/SDKs:ro \
    --workdir /project \
    -u $(id -u ${USER}):$(id -g ${USER}) \
    kushview/element
