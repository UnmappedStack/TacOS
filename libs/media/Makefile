CC = gcc
AR = ar
RM = rm -rf

LIBC = ../../libc
HEADERS = $(LIBC)/include

CFLAGS = -ffreestanding -I$(HEADERS) -Iinclude -static -nostdlib \
         -fno-stack-protector -fno-pie -g -mno-tls-direct-seg-refs

SRC_DIR = src
OBJ_DIR = obj
OUT = libmedia.a

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean dirs

all: $(OUT)

# Archive object files into a static library
$(OUT): $(OBJS)
	$(AR) rcs $@ $^

# Compile each .c to obj/.o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(CC) $(CFLAGS) -c $< -o $@

# ensure obj dir exists
dirs:
	@mkdir -p $(OBJ_DIR)

clean:
	$(RM) $(OBJ_DIR) $(OUT)
