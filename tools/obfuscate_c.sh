#!/bin/bash

SCRIPT=`realpath $0`
SCRIPT_PATH=`dirname $SCRIPT`

BASE_DIR="$SCRIPT_PATH/../obfuscate_code_generated"
SRC_DIR="$BASE_DIR/srcs"
INC_DIR="$BASE_DIR/includes"
ROOT="$SCRIPT_PATH/.."

rm -rf $BASE_DIR
mkdir $BASE_DIR
cp -rf $ROOT/Makefile $ROOT/srcs $ROOT/includes $BASE_DIR

DUMB_NB=0
FILE_DUMB_CONTENT=""
FILE_DUMB_PATH="$INC_DIR/dumb.h"

MAX_POSSIBLE_DUMB_FUNC=30
MAX_POSSIBLE_INSTRUCTIONS_IN_DUMB_FUNC=30
MAX_POSSIBLE_OP_PER_INSTRUCTIONS_IN_DUMB_FUNC=20

declare -a ARR_ARG=(
	"a" \
	"b" \
	"c" \
)

declare -a ARR_OP=(
	"+" \
	"-" \
	"*" \
	# "/" \ Segfault if / 0
	# "%" \
)

declare -a ARR_OP_ASSIGN=(
	"=" \
	"-=" \
	"+=" \
	"*=" \
	# "/=" \
	# "%=" \
	# "<<=" \
	# ">>=" \
)

declare -a ARR_SYSCALL=(
	"read" \
	"write" \
	"close" \
	"lseek" \
	"getdents" \
)

inc_var () {
	echo $(($1+1))
}

generate_random_arg () {
	local rand_offset=$(generate_random_nb 0 ${#ARR_ARG[@]})
	echo "${ARR_ARG[$rand_offset]}"
}

generate_random_op () {
	local rand_offset=$(generate_random_nb 0 ${#ARR_OP[@]})
	echo "${ARR_OP[$rand_offset]}"
}

generate_random_op_assign () {
	local rand_offset=$(generate_random_nb 0 ${#ARR_OP_ASSIGN[@]})
	echo "${ARR_OP_ASSIGN[$rand_offset]}"
}

generate_random_syscall () {
	local rand_offset=$(generate_random_nb 0 ${#ARR_SYSCALL[@]})
	echo "${ARR_SYSCALL[$rand_offset]}"
}

generate_random_nb () { # min max
	echo $(( $RANDOM % $2 + $1 ))
}

generate_random_line_op () {
	local line
	local rand_cond=$(generate_random_nb 0 2)

	if [ $rand_cond -eq 0 ] ; then
		line="syscall_wrapper(__NR_$(generate_random_syscall), -1"
		for ((i_gen_rand_line_op = 0 ; i_gen_rand_line_op <= 5 ; i_gen_rand_line_op++)); do
			rand_cond=$(generate_random_nb 0 3)
			if [ $rand_cond -eq 0 ] ; then
				line+=", $(generate_random_nb 0 0xffffffff)"
			else
				line+=", $(generate_random_arg)"
			fi
		done
		echo "$line);"
		return
	fi

	local rand_arg=$(generate_random_arg)
	local nb_iter=$(generate_random_nb 1 $MAX_POSSIBLE_OP_PER_INSTRUCTIONS_IN_DUMB_FUNC )
	line="$rand_arg $(generate_random_op_assign) $(generate_random_nb 0 0xffffffff)"
	for ((i_gen_rand_line_op = 0 ; i_gen_rand_line_op <= $nb_iter ; i_gen_rand_line_op++)); do
		rand_cond=$(generate_random_nb 0 2)
		if [ $rand_cond -eq 0 ] ; then
			line+=" $(generate_random_op) $(generate_random_arg)"
		else
			line+=" $(generate_random_op) $(generate_random_nb 0 0xffffffff)"
		fi
	done

	echo "$line;"
}

generate_dumb () {
	local dumb_proto

	FILE_DUMB_CONTENT='#ifndef DUMB_H\n# define DUMB_H\n# include <stdint.h>\n# include <unistd.h>\n# include <sys/syscall.h>\nlong	syscall_wrapper(long number, ...);\n\n'

	local nb_iter_new_dumb=$(generate_random_nb 1 $MAX_POSSIBLE_DUMB_FUNC )
	for ((i_new_dumb = 0 ; i_new_dumb <= $nb_iter_new_dumb ; i_new_dumb++)); do
		dumb_name=""
		dumb_proto="static inline __attribute__((always_inline)) void dumb${DUMB_NB}(size_t a __attribute__((unused)), size_t b __attribute__((unused)), size_t c __attribute__((unused)) ) {\n"
		FILE_DUMB_CONTENT+="$dumb_proto"
		local nb_iter_line_dumb=$(generate_random_nb 0 $MAX_POSSIBLE_INSTRUCTIONS_IN_DUMB_FUNC )
		for ((i_line_dumb = 0 ; i_line_dumb <= $nb_iter_line_dumb ; i_line_dumb++)); do
			FILE_DUMB_CONTENT+="\t$(generate_random_line_op)\n"
		done
		FILE_DUMB_CONTENT+="}\n\n"
		DUMB_NB=$(inc_var $DUMB_NB)
	done

	FILE_DUMB_CONTENT+="#endif\n"
	echo -ne "$FILE_DUMB_CONTENT" > "$FILE_DUMB_PATH"
	sed -i '/# define PESTILENCE_H/a # include "dumb.h"' $INC_DIR/pestilence.h
}


modify_makefile () {
	sed -i '/-DPAGE_SIZE=\$(PAGE_SIZE)/s/$/ -Wno-integer-overflow/' "$BASE_DIR/Makefile"
}

insert_fake_instructions() {
	local func_proto
	local arr_args

	for source_file in $SRC_DIR/[^dumb]*.c
	do
		local args_raw=$(cat "$source_file" | sed -nE 's/^(\w+)(\s+)(.+)\((.*)\)(\s+)\{$/\4/p')
		local functions_protos=$(cat "$source_file" | sed -nE 's/^(\w+)(\s+)(.+)\((.*)\)(\s+)\{$/\3(\4)\5{/p')
		local n_line_functions_protos=1

		while read -r line
		do
			local args_name_inside_func=()
			if [[ -z "$line" || "$line" == "void" ]] ; then
				continue
			fi

			# Fill array arg_name (ex: "void func(int a, int *b) {" --> [a, b])
			IFS=',' read -ra arr_args <<< "$line"
			for arg in "${arr_args[@]}"
			do

				IFS=' ' read -ra arr_arg <<< "$arg"
				arg_name="${arr_arg[${#arr_arg[@]}-1]}"
				arg_name=$(echo "$arg_name" | sed 's/*//g')
				args_name_inside_func+=("$arg_name")
			done

			# Generate random dumb func
			local rand_call_func=$(generate_random_nb 0 $DUMB_NB )
			local call_func_insert="  dumb$rand_call_func("
			for ((i_nb_arg = 0 ; i_nb_arg <= 2 ; i_nb_arg++)); do
				if [ $i_nb_arg -gt 0 ]; then
					call_func_insert+=", "
				fi

				rand_cond=$(generate_random_nb 0 2)
				if [ $rand_cond -eq 0 ]; then
					rand_index_arg=$(generate_random_nb 0 ${#args_name_inside_func[@]})
					rand_arg="${args_name_inside_func[$rand_index_arg]}"
					call_func_insert+="(size_t)$rand_arg"
				else
					rand_nb=$(generate_random_nb 0 0xffffffff)
					call_func_insert+="$rand_nb"
				fi
			done
			call_func_insert+=");"
			func_proto=$(sed -n "${n_line_functions_protos}p" <<< "$functions_protos" )
			func_proto=$(sed 's/\*/\\\*/g' <<< "$func_proto" )
			sed -i '/'"${func_proto}"'/a '"${call_func_insert}" $source_file
			n_line_functions_protos=$(inc_var $n_line_functions_protos)
		done <<< "$args_raw"
	done
}

modify_makefile
generate_dumb
insert_fake_instructions

exit 0
