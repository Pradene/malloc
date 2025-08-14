ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

NAME     = libft_malloc_$(HOSTTYPE).so
LINK     = libft_malloc.so
CC       = cc
CFLAGS   = -Wall -Wextra -Werror -fPIC -pthread -Iinc -I$(LIBFT_DIR)/inc

SRCS_DIR = src
OBJS_DIR = obj
TEST_DIR = test

LIBFT_DIR = libft
LIBFT     = $(LIBFT_DIR)/libft.a

SRCS      = $(wildcard $(SRCS_DIR)/*.c)
OBJS      = $(patsubst $(SRCS_DIR)/%.c,$(OBJS_DIR)/%.o,$(SRCS))

TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(patsubst $(TEST_DIR)/%.c,$(TEST_DIR)/%,$(TEST_SRCS))

all: $(LIBFT) $(NAME)
	@echo "\033[1;32m[OK]\033[0m Build complete: $(NAME)"

$(LIBFT):
	@echo "\033[1;34m[LIBFT]\033[0m Building libft with -fPIC"
	@$(MAKE) -C $(LIBFT_DIR) CFLAGS="$(CFLAGS)"

$(NAME): $(OBJS)
	@echo "\033[1;34m[LINK]\033[0m Creating shared library: $(NAME)"
	@$(CC) $(CFLAGS) $(OBJS) $(LIBFT) -shared -o $(NAME)
	@ln -sf $(NAME) $(LINK)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c
	@mkdir -p $(OBJS_DIR)
	@echo "\033[1;36m[CC]\033[0m $<"
	@$(CC) $(CFLAGS) -c $< -o $@

test: $(LIBFT) $(NAME) $(TEST_BINS)
	@for t in $(TEST_BINS); do \
		echo "\033[1;33m[RUN]\033[0m $$t (with LD_PRELOAD=$(NAME))"; \
		LD_PRELOAD=./$(NAME) $$t; \
	done

$(TEST_DIR)/%: $(TEST_DIR)/%.c $(NAME) $(LIBFT)
	@echo "\033[1;36m[CC-TEST]\033[0m $<"
	@$(CC) $(CFLAGS) $< -L. -lft_malloc_$(HOSTTYPE) -o $@

compile:
	@if [ -z "$(file)" ]; then \
		echo "Usage: make compile file=path/to/file.c [out=output_binary]"; \
	else \
		echo "\033[1;36m[CC-SINGLE]\033[0m $(file)"; \
		$(CC) $(CFLAGS) $(file) -L. -lft_malloc_$(HOSTTYPE) -o $${out:-a.out}; \
		echo "\033[1;33m[RUN]\033[0m $${out:-a.out} (with LD_PRELOAD=$(NAME))"; \
		LD_PRELOAD=./$(NAME) ./$${out:-a.out}; \
	fi

clean:
	@echo "\033[1;31m[CLEAN]\033[0m Removing object files"
	@rm -rf $(OBJS_DIR)
	@$(MAKE) -C $(LIBFT_DIR) clean

fclean: clean
	@echo "\033[1;31m[FCLEAN]\033[0m Removing binaries and symlinks"
	@rm -f $(NAME) $(LINK) $(TEST_BINS)
	@$(MAKE) -C $(LIBFT_DIR) fclean

re: fclean all

.PHONY: all clean fclean re test compile

