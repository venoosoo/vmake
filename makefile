CC = clang
CFLAGS = -Wall -O3 -Iinclude

SRC_DIR = src
OBJ_DIR = obj

SRC = $(SRC_DIR)/main.c \
      $(SRC_DIR)/parser.c \
      $(SRC_DIR)/dbgtools.c \
      $(SRC_DIR)/scanner.c \
      $(SRC_DIR)/cache.c \
      $(SRC_DIR)/xxhash.c \
	  $(SRC_DIR)/compile.c

OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

TARGET = vmake

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
re: clean all