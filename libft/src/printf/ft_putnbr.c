#include "libft.h"

static int put_digits(unsigned int num) {
	int printed = 0;

	if (num >= 10) {
		printed += put_digits(num / 10);
		ft_putchar('0' + num % 10);
	} else {
		ft_putchar('0' + num);
	}
	return (printed + 1);
}

int ft_putnbr_formatted(int n, t_printformat *fmt) {
	int printed = 0;
	int is_negative = (n < 0);
	unsigned int num = (is_negative) ? -n : n;
	int len = 0;
	unsigned int temp = num;
	int sign_len = (is_negative || fmt->plus || fmt->space) ? 1 : 0;
	
	// Calculate digit length
	len = (num == 0) ? 1 : 0;
	while (temp) {
		len++;
		temp /= 10;
	}
	
	int total_len = len + sign_len;
	int pad = fmt->width - total_len;
	
	// Handle sign and padding
	if (!fmt->minus) {
		if (fmt->zero) {
			if (is_negative) {
				ft_putchar('-');
				printed += 1;
			} else if (fmt->plus) {
				ft_putchar('+');
				printed += 1;
			} else if (fmt->space) {
				ft_putchar(' ');
				printed += 1;
			}
			printed += format_print_padding(pad, '0');
		} else {
			printed += format_print_padding(pad, ' ');
			if (is_negative) {
				ft_putchar('-');
				printed += 1;
			} else if (fmt->plus) {
				ft_putchar('+');
				printed += 1;
			} else if (fmt->space) {
				ft_putchar(' ');
				printed += 1;
			}
		}
	} else {
		if (is_negative) {
			ft_putchar('-');
			printed += 1;
		} else if (fmt->plus) {
			ft_putchar('+');
			printed += 1;
		} else if (fmt->space) {
			ft_putchar(' ');
			printed += 1;
		}
	}
	
	// Print digits
	if (num == 0) {
		ft_putchar('0');
		printed += 1;
	} else {
		printed += put_digits(num);
	}
	
	// Left alignment padding
	if (fmt->minus && pad > 0) {
		printed += format_print_padding(pad, ' ');
	}
	
	return printed;
}