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

PROJECT	= famine
NAME	= famine

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

CFLAGS = -Wall -Wextra -Werror -g #-fsanitize=address

ifeq ($(with-asm),y)
	WITH-ASM:= with-asm
endif

ROOT  	=	$(shell /bin/pwd)
OPATH 	=	$(ROOT)/objs
CPATH 	=	$(ROOT)/srcs
HPATH 	=	-I $(ROOT)/includes -I $(LIBFT)/includes

SRC =	famine.c \
	debug.c \
	elf.c \
	elf_read.c \
	elf_write.c \
	elf_modify.c \
	elf_save.c

ASMPATH		= $(ROOT)/asm
ASM_SRC		= $(ASMPATH)/unpacker.asm
ASM_OBJ		= $(ASMPATH)/unpacker.o
ASM_BYTECODE	= $(ASMPATH)/bytecode

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
.SILENT: $(NAME) $(OPATH) $(ASM_OBJ) $(OPATH)/%.o clean fclean re

all: $(NAME)

$(ASM_OBJ):
	$(NASM) -f elf64 $(ASM_SRC) -o $(ASM_OBJ)
	$(OBJCOPY) -O binary -j .text $(ASM_OBJ) $(ASM_BYTECODE)

$(NAME): $(ASM_OBJ) $(OPATH) $(OBJ)
	$(if $(filter $(COMPILE),yes),echo ']')
	printf $(PROJECT)": Building $@ ... "
	$(CC) -o $@ $(CFLAGS) $(OBJ) $(LPATH) $(HPATH) $(ASM_OBJ)
	$(call PRINT_STATUS,DONE,SUCCESS)

$(OPATH)/%.o: $(CPATH)/%.c | $(PRE_CHECK_LIB)
	$(if $(filter $(COMPILE),no),@printf $(PROJECT)': Files compiling [')
	$(eval COMPILE := yes)
	@$(CC) $(CFLAGS) -c $< -o $@ $(HPATH) -DBYTECODE=\"`$(HEXDUMP) -v -e '"\\\\x" 1/1 "%02X"' $(ASM_BYTECODE)`\"
	@$(call PRINT_GREEN,.)

$(OPATH):
	echo $(PROJECT)": Creation of objects directory"
	$(MKDIR) $@

clean:
	$(RM) -Rf $(OPATH)
	$(RM) -f $(ASM_OBJ)
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
