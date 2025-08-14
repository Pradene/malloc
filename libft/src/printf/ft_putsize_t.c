#include "libft.h"

int ft_putsize_t_formatted(size_t n, t_printformat *fmt) {	
	// Calculate length
	size_t temp = n;
	int len = 0;
	if (temp == 0) {
		len = 1;
	} else {
		while (temp) {
			temp /= 10;
			len++;
		}
	}
	
	int size = 0;
	
	// Handle padding
	if (fmt->width > len && !fmt->minus) {
		char pad_char = fmt->zero ? '0' : ' ';
		while (size + len < fmt->width) {
			ft_putchar(pad_char);
			size++;
		}
	}

	char buf[21];
	int i = 20;
	
	buf[i] = '\0';
	
	if (n == 0) {
		buf[--i] = '0';
	} else {
		while (n) {
			buf[--i] = (n % 10) + '0';
			n /= 10;
		}
	}
	
	ft_putstr(&buf[i]);
	size += ft_strlen(&buf[i]);
	
	// Left align padding
	if (fmt->minus && fmt->width > size) {
		while (size < fmt->width) {
			ft_putchar(' ');
			size++;
		}
	}
	
	return (fmt->width > size ? fmt->width : size);
}