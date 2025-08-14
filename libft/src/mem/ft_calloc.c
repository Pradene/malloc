#include "libft.h"

void	*ft_calloc(size_t n, size_t size) {
	void	*ptr;

	if (n == 0 || size == 0) {
		return (malloc(0));
	}
	if (n * size < n) {
		return (0);
	}
	ptr = malloc(n * size);
	if (!ptr) {
		return (0);
	}
	ft_bzero(ptr, n * size);
	return (ptr);
}
