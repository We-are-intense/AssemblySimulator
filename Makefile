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

# CFLAGS = -Wall -ggdb -Werror -std=gnu99 -Wno-unused-function
CFLAGS = -ggdb -std=gnu99

EXECUTABLE = csapp

SRC_DIR = ./src
OUTPUT_DIR = ./build
INCS := $(foreach dir,$(SRC_DIR),-I$(dir))
code_dirs = $(SRC_DIR)/cpu $(SRC_DIR)/memory $(SRC_DIR)/disk $(SRC_DIR)
code_srcs = $(foreach dir, $(code_dirs), $(wildcard $(dir)/*.c))
code_objs = $(patsubst %.c,$(OUTPUT_DIR)/%.o,$(code_srcs))


$(EXECUTABLE) : $(code_objs)
	$(CC) $(code_objs) -o $@

# -MMD -MP 让编译器顺带生成 .d 依赖文件，头文件改动后引用它的 .o 会自动重编，
# 否则改了结构体布局（如 register.h）而旧 .o 仍按旧偏移访问，运行结果会错
$(OUTPUT_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -I$(SRC_DIR) -c $< -o $@

-include $(code_objs:.o=.d)


run:
	./$(EXECUTABLE)

# 测试：test/ 下每个 .c 编译成一个独立可执行文件，依次运行，任一失败即退出非 0
# 测试文件会 #include 被测的 .c 以访问 static 函数，所以链接时排除 main.o 和 instruction.o
TEST_DIR  = ./test
test_srcs = $(wildcard $(TEST_DIR)/*.c)
test_bins = $(patsubst $(TEST_DIR)/%.c,$(OUTPUT_DIR)/test/%,$(test_srcs))
test_objs = $(filter-out %/main.o %/instruction.o,$(code_objs))

test: $(test_bins)
	@for t in $(test_bins); do echo "== $$t"; $$t || exit 1; done

$(OUTPUT_DIR)/test/%: $(TEST_DIR)/%.c $(test_objs) $(wildcard $(SRC_DIR)/*/*.c $(SRC_DIR)/*/*.h)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $< $(test_objs) -o $@

.PHONY: clean run test
clean:
	rm -f *.o *~ $(EXECUTABLE)
	rm -r $(OUTPUT_DIR)

