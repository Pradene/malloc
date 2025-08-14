#include "libft.h"

void	*ft_memset(void *s, int value, size_t n) {
	size_t	i;

	i = 0;
	while (i < n) {
		((char *)s)[i++] = value;
	}
	return (s);
}
