SYSTEM := $(shell uname)

ifeq ($(SYSTEM), Windows)
    $(info Current operating system is Windows)
    # Windows-specific commands or variables here
else ifeq ($(SYSTEM), Linux)
    $(info Current operating system is Linux)
    # Linux-specific commands or variables here
	CC = /usr/bin/gcc
else ifeq ($(SYSTEM), Darwin)
    $(info Current operating system is macOS)
    # macOS-specific commands or variables here
	CC = /usr/bin/clang
else
    $(error Unknown operating system)
endif

CFLAGS = -Wall -g -O2 -Werror -std=gnu99 -Wno-unused-function

EXE_HARDWARE = exe_hardware

SRC_DIR = ./src

# main
MAIN_HARDWARE = $(SRC_DIR)/main.c

.PHONY:main
main:
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(MAIN_HARDWARE) -o $(EXE_HARDWARE)

run:
	./$(EXE_HARDWARE)


clean:
	rm -f *.o *~ $(EXE_HARDWARE)