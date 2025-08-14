#include "libft.h"

int ft_putchar_formatted(char c, t_printformat *fmt) {
	int printed = 0;
	int padding = fmt->width - 1;
	
	if (padding > 0) {
		if (fmt->minus) {
			ft_putchar(c);
			printed += format_print_padding(padding, ' ');
		} else {
			printed += format_print_padding(padding, ' ');
			ft_putchar(c);
		}
	} else {
		ft_putchar(c);
	}
	return (printed + 1);
}