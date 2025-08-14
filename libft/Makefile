SRC_DIR := src
OBJ_DIR := obj

SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

CC     := cc
CFLAGS := -Wall -Wextra -Werror -fPIC -Iinc

NAME := libft.a

all: $(NAME)
	@echo "\033[1;32m[OK]\033[0m Build complete: $(NAME)"

$(NAME): $(OBJS)
	@echo "\033[1;34m[AR]\033[0m $@"
	@ar rc $(NAME) $(OBJS)
	@ranlib $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "\033[1;36m[CC]\033[0m $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "\033[1;31m[CLEAN]\033[0m Removing object files"
	@rm -rf $(OBJ_DIR)

fclean: clean
	@echo "\033[1;31m[FCLEAN]\033[0m Removing $(NAME)"
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

