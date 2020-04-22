#!/bin/bash

FORBIDDEN_PROCESS="sleep"
FORBIDDEN_PROCESS_ARGS="30"
TARGET_DIR="/tmp/test"
TARGET="/bin/ls"
SIGNATURE="vsteffen"

SCRIPT=$( realpath $0 )
SCRIPT_PATH=$( dirname $SCRIPT )
PESTILENCE_PATH="$SCRIPT_PATH/../pestilence"

if ! [ -x "$(command -v $PESTILENCE_PATH)" ]; then
	make re -C "$SCRIPT_PATH/.." DEBUG=y
fi

if ! [ -z "$(pidof $FORBIDDEN_PROCESS)" ]; then
	killall -9 "$FORBIDDEN_PROCESS"
fi

generate_new_target() {
	rm -rf "$1"
	mkdir "$1"
	cp "$2" "$1"
}

execute_pestilence_and_check_signature () {
	local target_new_path="$TARGET_DIR/$(basename $TARGET)"

	$PESTILENCE_PATH

	local res_string=$( strings "$target_new_path" | grep "$SIGNATURE" )
	if [ -z "$res_string" ]; then
		echo "Signature not found in $TARGET"
	else
		echo "Signature found in $TARGET --> $res_string"
	fi
}

echo "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+"
echo "Execute $(basename $PESTILENCE_PATH) without forbidden process"

generate_new_target "$TARGET_DIR" "$TARGET"
execute_pestilence_and_check_signature "$TARGET"

echo "---------------------------------------------"
echo "Execute $(basename $PESTILENCE_PATH) with forbidden process"

$FORBIDDEN_PROCESS $FORBIDDEN_PROCESS_ARGS &
FORBIDDEN_PROCESS_PID=$!

generate_new_target "$TARGET_DIR" "$TARGET"
execute_pestilence_and_check_signature "$TARGET"

echo "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+"

kill -9 "$FORBIDDEN_PROCESS_PID"