#!/bin/bash

IMAGE_NAME="pestilence"
SCRIPT_PATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

if [[ "$(docker images -q famine 2> /dev/null)" == "" ]]; then
	docker build -t "$IMAGE_NAME" "$SCRIPT_PATH"
	if [ $? -ne 0 ]; then
		exit 1
	fi
fi

docker run --cap-add=SYS_PTRACE --security-opt seccomp=unconfined -it --mount type=bind,source="$SCRIPT_PATH",target=/root/pestilence pestilence
