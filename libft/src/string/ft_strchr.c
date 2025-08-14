#include "libft.h"

char	*ft_strchr(const char *str, int c) {
	int	i;

	if (!str) {
		return (0);
	}
	i = 0;
	while (i < ft_strlen(str) + 1) {
		if (str[i] == (char)c) {
			return ((char *)str + i);
		}
		i++;
	}
	return (0);
}
