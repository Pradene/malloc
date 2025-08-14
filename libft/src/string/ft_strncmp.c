#include "libft.h"

int	ft_strncmp(const char *str1, const char *str2, size_t n) {
	size_t	i;

	i = 0;
	while (i < n) {
		if ((unsigned char)str1[i] != (unsigned char)str2[i]) {
			return ((unsigned char)str1[i] - (unsigned char)str2[i]);
		}
		i++;
	}
	return (0);
}
