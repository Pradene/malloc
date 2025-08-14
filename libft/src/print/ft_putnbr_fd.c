#include "libft.h"

void	ft_putnbr_fd(int n, int fd) {
	char	d;

	if (n == -2147483648) {
		write(fd, "-2147483648", 11);
	} else if (n < 0 && n != -2147483648) {
		write(fd, "-", 1);
		n *= -1;
	}
	if (n >= 10) {
		d = n % 10 + 48;
		n /= 10;
		ft_putnbr_fd(n, fd);
		write(fd, &d, 1);
	} else if (n >= 0 && n < 10) {
		d = n + 48;
		write(fd, &d, 1);
	}
}
