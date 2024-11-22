# 编译器和编译选项
CC = gcc
CFLAGS = -Wall -g

# 源文件和目标文件
SRCS = db.c request_handler.c server.c
OBJS = $(SRCS:.c=.o)
TARGET = server  # 最终生成的可执行文件名为 server

# 头文件目录（如果有多个目录可以添加）
INCLUDE_DIR = .

# 默认目标：编译并链接可执行文件
all: $(TARGET)

# 链接目标文件生成可执行文件，并链接 SQLite3 库
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ -lsqlite3

# 编译规则：将 .c 文件编译成 .o 文件
%.o: %.c head.h
	$(CC) -c $(CFLAGS) -I$(INCLUDE_DIR) $< -o $@

# 清理生成的文件
clean:
	rm -f $(OBJS) $(TARGET)

# 伪目标（避免与文件名冲突）
.PHONY: all clean
