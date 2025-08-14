#include "libft.h"

int	ft_atoi(const char *str) {
	int	i;
	int	value;
	int	sign;

	i = 0;
	value = 0;
	sign = 1;
	while (ft_isspace(str[i])) {
		i++;
	}
	if (str[i] == '-') {
		sign *= -1;
	}
	if (str[i] == '-' || str[i] == '+') {
		i++;
	}
	while (ft_isdigit(str[i])) {
		value = value * 10 + str[i++] - 48;
	}
	return (value * sign);
}
