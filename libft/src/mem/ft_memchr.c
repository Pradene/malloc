#include "libft.h"

void	*ft_memchr(const void *mem, int c, size_t n) {
	size_t	i;

	if (!mem) {
		return (0);
	}
	i = 0;
	while (i < n) {
		if (((char *)mem)[i] == (char)c) {
			return ((char *)mem + i);
		}
		i++;
	}
	return (0);
}
