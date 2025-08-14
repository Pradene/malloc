# Directories
SRC_DIR := src
OBJ_DIR := obj

# Find all .c files recursively
SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')

# Convert src paths to obj paths
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Compiler settings
CC := gcc
CFLAGS := -Wall -Wextra -Werror -Iinc

# Library name
NAME := libft.a

# Default target
all: $(NAME)

# Rule for building the library
$(NAME): $(OBJS)
	@echo "AR $@"
	@ar rc $(NAME) $(OBJS)
	@ranlib $(NAME)

# Compile each .c into .o, creating directories as needed
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Cleaning
clean:
	@echo "Cleaning object files..."
	@rm -rf $(OBJ_DIR)

fclean: clean
	@echo "Removing $(NAME)..."
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

