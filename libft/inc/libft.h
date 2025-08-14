#ifndef LIBFT_H
# define LIBFT_H

# include <stdlib.h>
# include <unistd.h>
# include <stdarg.h>
# include <stdbool.h>
# include <stddef.h>

typedef struct s_list {
	void			*content;
	struct s_list	*next;
}	t_list;

// Structure to hold format flags and width for printf
typedef struct s_printformat {
	int minus;	  // '-' flag (left justify)
	int zero;	   // '0' flag (zero padding)
	int hash;	   // '#' flag (alternate form)
	int space;	  // ' ' flag (space for positive numbers)
	int plus;	   // '+' flag (always show sign)
	int width;	  // minimum field width
} t_printformat;

// CHAR
int		ft_isalnum(int c);
int		ft_isspace(int c);
int		ft_isalpha(int c);
int		ft_isdigit(int c);
int		ft_isascii(int c);
int		ft_isprint(int c);
int		ft_toupper(int c);
int		ft_tolower(int c);

//STRING
int		ft_atoi(const char *str);
char	*ft_itoa(int n);
char	**ft_split(char const *s, char c);
char	*ft_strdup(const char *s);
int		ft_strlen(const char *str);
size_t	ft_strlcpy(char *dest, const char *src, size_t n);
size_t	ft_strlcat(char *dest, const char *src, size_t n);
char	*ft_strchr(const char *str, int c);
char	*ft_strrchr(const char *str, int c);
int		ft_strncmp(const char *first, const char *second, size_t n);
char	*ft_strnstr(const char *s1, const char *s2, size_t n);
char	*ft_strjoin(char const *s1, char const *s2);
char	*ft_strtrim(char const *s1, char const *set);
char	*ft_substr(const char *s, unsigned int start, size_t n);

// MEM
void	ft_bzero(void *ptr, size_t n);
void	*ft_calloc(size_t n, size_t size);
void	*ft_memset(void *s, int value, size_t n);
void	*ft_memcpy(void *dest, const void *src, size_t n);
void	*ft_memmove(void *dest, const void *src, size_t n);
void	*ft_memchr(const void *mem, int c, size_t n);
int		ft_memcmp(const void *mem1, const void *mem2, size_t n);
char	*ft_strmapi(char const *s, char (*f)(unsigned int, char));
void	ft_striteri(char *s, void (*f)(unsigned int, char *));

// PRINT
void	ft_putchar_fd(char c, int fd);
void	ft_putchar(char c);
void	ft_putstr_fd(char *s, int fd);
void	ft_putstr(char *s);
void	ft_putendl_fd(char *s, int fd);
void	ft_putnbr_fd(int n, int fd);

// LIST
t_list	*ft_lstnew(void *content);
void	ft_lstadd_front(t_list **lst, t_list *new);
int		ft_lstsize(t_list *lst);
t_list	*ft_lstlast(t_list *lst);
void	ft_lstadd_back(t_list **lst, t_list *new);
void	ft_lstdelone(t_list *lst, void (*del)(void*));
void	ft_lstclear(t_list **lst, void (*del)(void*));
void	ft_lstiter(t_list *lst, void (*f)(void *));
t_list	*ft_lstmap(t_list *lst, void *(*f)(void *), void (*del)(void *));

// PRINTF
int	ft_printf(const char *s, ...);
int	ft_format(va_list params, t_printformat *fmt, char c);
int	ft_putstr_formatted(char *s, t_printformat *fmt);
int	ft_putchar_formatted(char c, t_printformat *fmt);
int	ft_puthex_formatted(unsigned long long n, t_printformat *fmt, char format);
int	ft_puthex(unsigned long long n, char format);
int	ft_putnbr_formatted(int n, t_printformat *fmt);
int	ft_putunbr_formatted(unsigned int n, t_printformat *fmt);
int	ft_putptr_formatted(unsigned long long n, t_printformat *fmt);
int	ft_putsize_t_formatted(size_t n, t_printformat *fmt);

void format_init(t_printformat *fmt);
void format_parse_flags(const char *s, int *i, t_printformat *fmt);
int	format_print_padding(int padding, char c);

#endif
