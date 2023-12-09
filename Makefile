CC = /usr/bin/gcc
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