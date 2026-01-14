#!/bin/sh
# SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs the kushview/element docker image. By default it starts
# a bash shell.

entry="${1}"
if [ -z "${entry}" ]; then
    entry="/bin/bash"
fi

extraopts=""
if [ -d "$HOME/SDKs" ]; then
    extraopts="$extraopts --volume ${HOME}/SDKs:/SDKs:ro"
fi

mkdir -p dist

docker run --rm -it \
    --entrypoint "${entry}" \
    --volume `pwd`/dist:/dist \
    --volume `pwd`:/project \
    --workdir /project \
    $extraopts \
    -u $(id -u ${USER}):$(id -g ${USER}) \
    kushview/element
