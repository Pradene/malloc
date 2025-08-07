ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

NAME     = libft_malloc_$(HOSTTYPE).so
CC       = cc
CFLAGS   = -Wall -Wextra -Werror -Iinc -g
SRCS_DIR = src
OBJS_DIR = obj
TEST_DIR = test

SRCS     = $(wildcard $(SRCS_DIR)/*.c)
OBJS     = $(patsubst $(SRCS_DIR)/%.c,$(OBJS_DIR)/%.o,$(SRCS))

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -shared -o $(NAME)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c
	@mkdir -p $(OBJS_DIR)
	$(CC) $(CFLAGS) -c -fPIC $< -o $@

clean:
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
