#include "libft.h"

static int	absolute(int n) {
	if (n < 0)
		return (-n);
	return (n);
}

static int	int_len(int n) {
	int	c;
	int	d;

	d = 1;
	c = 0;
	if (n < 0) {
		c++;
	}
	while (n || d == 1) {
		d = 0;
		n /= 10;
		c++;
	}
	return (c);
}

char	*ft_itoa(int n) {
	int		c;
	char	*str;
	int		d;

	d = 1;
	c = int_len(n);
	str = ft_calloc(c + 1, sizeof(char));
	if (!str) {
		return (0);
	}
	if (n < 0) {
		*str = '-';
	}
	while (n || d == 1) {
		d = 0;
		c--;
		str[c] = absolute(n % 10) + 48;
		n /= 10;
	}
	return (str);
}
