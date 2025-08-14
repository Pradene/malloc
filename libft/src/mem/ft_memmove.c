#include "libft.h"

void	*ft_memmove(void *dest, const void *src, size_t n) {
	size_t	i;

	i = 0;
	if (dest > src) {
		i = n;
		while (i) {
			--i;
			((char *)dest)[i] = ((char *)src)[i];
		}
	} else {
		while (i < n) {
			((char *)dest)[i] = ((char *)src)[i];
			++i;
		}
	}
	return (dest);
}
