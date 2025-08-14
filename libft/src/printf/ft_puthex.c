#include "libft.h"

static int put_hex_digits(unsigned long long n, char c) {
	char digits[] = "0123456789abcdef";
	if (c == 'X') {
		digits[10] = 'A';
		digits[11] = 'B';
		digits[12] = 'C';
		digits[13] = 'D';
		digits[14] = 'E';
		digits[15] = 'F';
	}
	
	if (n >= 16) {
		return (put_hex_digits(n / 16, c) + put_hex_digits(n % 16, c));
	} else {
		ft_putchar(digits[n]);
		return (1);
	}
}

int ft_puthex(unsigned long long n, char c) {
	if (n == 0) {
		ft_putchar('0');
		return (1);
	} else {
		return (put_hex_digits(n, c));
	}
}

int ft_puthex_formatted(unsigned long long n, t_printformat *fmt, char c) {
	int printed = 0;
	int len = 0;
	unsigned long long temp = n;
	int prefix_len = (fmt->hash && n != 0) ? 2 : 0;
	
	// Calculate digit length
	len = (n == 0) ? 1 : 0;
	while (temp) {
		len++;
		temp /= 16;
	}
	
	int total_len = len + prefix_len;
	int pad = fmt->width - total_len;
	
	// Padding for right alignment
	if (!fmt->minus && !fmt->zero && pad > 0)
		printed += format_print_padding(pad, ' ');
	
	// Prefix and zero padding
	if (prefix_len) {
		ft_putchar('0');
		ft_putchar(c == 'x' ? 'x' : 'X');
		printed += 2;
	}
	
	if (!fmt->minus && fmt->zero && pad > 0) {
		printed += format_print_padding(pad, '0');
	}
	
	// Print digits
	printed += ft_puthex(n, c);
	
	// Left alignment padding
	if (fmt->minus && pad > 0) {
		printed += format_print_padding(pad, ' ');
	}
	
	return (printed);
}