#include "libft.h"

int	ft_putstr_formatted(char *s, t_printformat *fmt) {
	int len;
	int size = 0;
	
	if (!s) {
		s = "(null)";
	}
	len = ft_strlen(s);
	if (fmt->width > len && !fmt->minus) {
		size += format_print_padding(fmt->width - len, ' ');
	}
	ft_putstr(s);
	size += len;
	if (fmt->width > len && fmt->minus) {
		while (size < fmt->width) {
			ft_putchar(' ');
			size++;
		}
	}
	return (fmt->width > len ? fmt->width : len);
}