#include "libft.h"

int ft_putptr_formatted(unsigned long long n, t_printformat *fmt) {
	int printed = 0;
	int hex_len = 0;
	unsigned long long temp = n;
	
	if (n == 0) {
		char *nil_str = "(nil)";
		int len = 5;
		int pad = fmt->width - len;
		
		// Right alignment padding
		if (!fmt->minus && pad > 0) {
			printed += format_print_padding(pad, ' ');
		}
		
		printed += write(1, nil_str, len);
		
		// Left alignment padding
		if (fmt->minus && pad > 0) {
			printed += format_print_padding(pad, ' ');
		}
	} else {
		// Calculate hex length
		temp = n;
		hex_len = (n == 0) ? 1 : 0;
		while (temp) {
			hex_len++;
			temp /= 16;
		}
		
		int total_len = hex_len + 2; // "0x" prefix
		int pad = fmt->width - total_len;
		
		// Right alignment padding (before entire pointer)
		if (!fmt->minus && pad > 0) {
			printed += format_print_padding(pad, fmt->zero ? '0' : ' ');
		}
		
		// Print prefix and value together
		printed += write(1, "0x", 2);
		printed += ft_puthex(n, 'x');
		
		// Left alignment padding (after entire pointer)
		if (fmt->minus && pad > 0) {
			printed += format_print_padding(pad, ' ');
		}
	}
	return (printed);
}