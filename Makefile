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

EXECUTABLE = csapp

SRC_DIR = ./src
OUTPUT_DIR = ./build
INCS := $(foreach dir,$(SRC_DIR),-I$(dir))
code_dirs = $(SRC_DIR)/cpu $(SRC_DIR)/memory $(SRC_DIR)
code_srcs = $(foreach dir, $(code_dirs), $(wildcard $(dir)/*.c))
code_objs = $(patsubst %.c,$(OUTPUT_DIR)/%.o,$(code_srcs))


$(EXECUTABLE) : $(code_objs)
	$(CC) $(code_objs) -o $@

$(OUTPUT_DIR)/%.o: %.c
	@echo "c=$< o=$@"
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@


run:
	./$(EXECUTABLE)

.PHONY: clean
clean:
	rm -f *.o *~ $(EXECUTABLE)
	rm -r $(OUTPUT_DIR) main

