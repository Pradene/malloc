#include "libft.h"

static int put_unsigned_digits(unsigned int n) {
	int printed = 0;
	if (n >= 10) {
		printed += put_unsigned_digits(n / 10);
		ft_putchar('0' + n % 10);
	} else {
		ft_putchar('0' + n);
	}
	return (printed + 1);
}

int ft_putunbr_formatted(unsigned int n, t_printformat *fmt) {
	int size = 0;
	int len = 0;
	unsigned int temp = n;
	
	// Calculate length
	len = (n == 0) ? 1 : 0;
	while (temp) {
		len++;
		temp /= 10;
	}
	
	int pad = fmt->width - len;
	
	// Right alignment padding
	if (!fmt->minus && pad > 0) {
		char pad_char = fmt->zero ? '0' : ' ';
		size += format_print_padding(pad, pad_char);
	}
	
	// Print number
	if (n == 0) {
		ft_putchar('0');
		size += 1;
	} else {
		size += put_unsigned_digits(n);
	}
	
	// Left alignment padding
	if (fmt->minus && pad > 0)
		size += format_print_padding(pad, ' ');
	
	return size;
}