# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: vsteffen <marvin@42.fr>                    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2016/02/06 20:52:05 by vsteffen          #+#    #+#              #
#    Updated: 2018/03/20 18:17:27 by vsteffen         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

PROJECT	= pestilence
NAME	= pestilence

CC 	=	/usr/bin/clang
RM 	=	/bin/rm
MAKE 	=	/usr/bin/make
MKDIR 	=	/bin/mkdir -p
AR 	=	/usr/bin/ar
RANLIB 	=	/usr/bin/ranlib
GIT	=	/usr/bin/git
NASM	=	/usr/bin/nasm
OBJCOPY	=	/usr/bin/objcopy
HEXDUMP	=	/usr/bin/hexdump

OBJ = $(patsubst %.c, $(OPATH)/%.o, $(SRC))

PAGE_SIZE= $(shell getconf PAGE_SIZE)

CFLAGS = -Wall -Wextra -Werror -fpic -nostdlib -fno-stack-protector -DPAGE_SIZE=$(PAGE_SIZE)

ifeq ($(with-asm),y)
	WITH-ASM:= with-asm
endif

ifeq ($(DEBUG),y)
	CFLAGS+= -DDEBUG=true
endif

ROOT  	=	$(shell /bin/pwd)
OPATH 	=	$(ROOT)/objs
CPATH 	=	$(ROOT)/srcs
HPATH 	=	-I $(ROOT)/includes

SRC =	elf.c \
	elf_read.c \
	elf_write.c \
	elf_modify.c \
	elf_save.c \
	tools.c \
	woody_mod.c \
	pestilence.c \
	famine.c

ASM_OBJ 	= $(patsubst %.s, $(ASM_OPATH)/%.o, $(ASM_SRC))
ASM_OPATH 	=	$(ROOT)/objs/asm
ASM_CPATH 	=	$(ROOT)/srcs/asm

ASM_SRC =	unpacker.s \
		syscall.s


OBJ_DIR		= $(OPATH) $(ASM_OPATH)

COMPILE	= no

OS	:= $(shell uname -s)
ifeq ($(OS),Darwin)
	NPROCS:=$(shell sysctl -n hw.ncpu)
else
	NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
endif


define PRINT_RED
	printf "\033[31m$(1)\033[0m"
endef

define PRINT_GREEN
	printf "\033[32m$(1)\033[0m"
endef

define PRINT_YELLOW
	printf "\033[33m$(1)\033[0m"
endef

define PRINT_STATUS
	printf '['
	$(if $(filter $(2),SUCCESS),$(call PRINT_GREEN,$(1)))
	$(if $(filter $(2),FAIL),$(call PRINT_RED,$(1)))
	$(if $(filter $(2),WARN),$(call PRINT_YELLOW,$(1)))
	$(if $(filter $(2),INFO),printf $(1))
	$(if $(filter $(3),-n),printf $(1),echo ']')
endef

.PHONY: all clean fclean re lib-clean lib-update
.SILENT: $(NAME) $(OBJ_DIR) $(ASM_OPATH) $(OPATH)/%.o clean fclean re

all: $(NAME)

$(NAME): $(OBJ_DIR) $(ASM_OBJ) $(OBJ)
	$(if $(filter $(COMPILE),yes),echo ']')
	printf $(PROJECT)": Building $@ ... "
	$(CC) -o $@ $(CFLAGS) $(ASM_OBJ) $(OBJ) $(HPATH)
	$(call PRINT_STATUS,DONE,SUCCESS)

$(ASM_OPATH)/%.o: $(ASM_CPATH)/%.s
	$(if $(filter $(COMPILE),no),@printf $(PROJECT)': Files compiling [')
	$(eval COMPILE := yes)
	@$(NASM) -f elf64 $< -o $@
	@$(call PRINT_GREEN,.)

$(OPATH)/%.o: $(CPATH)/%.c
	$(if $(filter $(COMPILE),no),@printf $(PROJECT)': Files compiling [')
	$(eval COMPILE := yes)
	@$(CC) $(CFLAGS) -c $< -o $@ $(HPATH)
	@$(call PRINT_GREEN,.)

$(OBJ_DIR):
	$(MKDIR) $@

clean:
	$(RM) -Rf $(OPATH)
	echo $(PROJECT)": Objects cleaned "
	printf $(PROJECT)": $@ rule "
	$(call PRINT_STATUS,DONE,SUCCESS)

fclean: clean
	$(RM) -f $(NAME)
	$(RM) -f $(NEW_BIN)
	$(RM) -f $(ASM_BYTECODE)
	echo $(PROJECT)": executable clean"
	printf $(PROJECT)": $@ rule "
	$(call PRINT_STATUS,DONE,SUCCESS)

re: fclean
	$(MAKE) -C $(ROOT) -j$(NPROCS) -s
