# Libft
Libft is a personal implementation of a standard C library, created as part of the 42 School curriculum. This project serves as the foundation for many subsequent assignments, providing essential functions for string manipulation, memory management, and more.

### Mandatory Functions
The mandatory functions cover a range of utilities:

Character Checks:
- ft_isalpha()
- ft_isdigit()
- ft_isalnum()
- ft_isascii()
- ft_isprint()

Character Conversion:
- ft_toupper()
- ft_tolower()

String Manipulation:
- ft_strlen()
- ft_strdup()
- ft_strlcpy()
- ft_strlcat()
- ft_strchr()
- ft_strrchr()
- ft_strnstr()
- ft_strncmp()
- ft_substr()
- ft_strjoin()
- ft_strtrim()
- ft_split()
- ft_itoa()
- ft_strmapi()
- ft_striteri()
  
Memory Manipulation:
- ft_memset()
- ft_bzero()
- ft_memcpy()
- ft_memmove()
- ft_memchr()
- ft_memcmp()

Conversion:
- ft_atoi()

Printing:
- ft_putchar_fd()
- ft_putstr_fd()
- ft_putendl_fd()
- ft_putnbr_fd()

### Bonus Functions
The bonus functions extend the library's capabilities with linked list operations:

- ft_lstnew()
- ft_lstadd_front()
- ft_lstsize()
- ft_lstlast()
- ft_lstadd_back()
- ft_lstdelone()
- ft_lstclear()
- ft_lstiter()
- ft_lstmap()

## Installation
To compile the library:

1. Clone the repository:

```bash
git clone https://github.com/Pradene/libft.git
```
   
2. Build the project:

```bash
make
```

3. To use the library in your projects, include the header file:
```c
#include "libft.h"
```

## Usage
After compilation, the library functions can be utilized in your C projects. Ensure that the libft.a static library is linked during the compilation of your projects.

