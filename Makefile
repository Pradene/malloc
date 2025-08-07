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

# Test files and executables
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(patsubst $(TEST_DIR)/%.c,$(TEST_DIR)/%,$(TEST_SRCS))

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -shared -o $(NAME)
	ln -sfT $(NAME) libft_malloc.so

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c
	@mkdir -p $(OBJS_DIR)
	$(CC) $(CFLAGS) -c -fPIC $< -o $@

# Test rule: compile each test file with the library
test: $(NAME) $(TEST_BINS)

$(TEST_DIR)/%: $(TEST_DIR)/%.c $(NAME)
	$(CC) $(CFLAGS) $< -L. -lft_malloc_$(HOSTTYPE) -o $@

clean:
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -f $(NAME)
	rm -f $(TEST_BINS)

re: fclean all

.PHONY: all clean fclean re test
