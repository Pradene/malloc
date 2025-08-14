#include "libft.h"

int	ft_putformated(va_list params, t_printformat *fmt, char c) {
	int	printed = 0;

	if (c == 'c') {
		printed += ft_putchar_formatted(va_arg(params, int), fmt);
	} else if (c == 's') {
		printed += ft_putstr_formatted(va_arg(params, char *), fmt);
	} else if (c == 'p') {
		printed += ft_putptr_formatted(va_arg(params, unsigned long long), fmt);
	} else if (c == 'd' || c == 'i') {
		printed += ft_putnbr_formatted(va_arg(params, int), fmt);
	} else if (c == 'u') {
		printed += ft_putunbr_formatted(va_arg(params, unsigned int), fmt);
	} else if (c == 'x' || c == 'X') {
		printed += ft_puthex_formatted(va_arg(params, unsigned int), fmt, c);
	} else if (c == 'z') {
		printed += ft_putsize_t_formatted(va_arg(params, size_t), fmt);
	} else if (c == '%') {
		ft_putchar('%');
		printed += 1;
	}
	return (printed);
}

int ft_printf(const char *s, ...) {
	va_list params;
	t_printformat fmt;
	int printed = 0;
	int i = 0;

	va_start(params, s);
	while (s[i]) {
		if (s[i] != '%') {
			ft_putchar(s[i]);
			printed += 1;
		} else {
			++i;
			format_init(&fmt);
			format_parse_flags(s, &i, &fmt);
			printed += ft_putformated(params, &fmt, s[i]);
		}
		++i;
	}
	va_end(params);
	return (printed);
}