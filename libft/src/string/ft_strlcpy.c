#include "libft.h"

size_t	ft_strlcpy(char *dest, const char *src, size_t n) {
	size_t	i;

	i = 0;
	while (i < n - 1 && n != 0) {
		if (src[i] != 0) {
			dest[i] = src[i];
			i++;
		} else {
			break ;
		}
	}
	if (n) {
		dest[i] = 0;
	}
	return (ft_strlen(src));
}
