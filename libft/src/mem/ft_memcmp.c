#include "libft.h"

static int	absolute(int n) {
	if (n < 0)
		return (-n);
	return (n);
}

int	ft_memcmp(const void *mem1, const void *mem2, size_t n) {
	size_t	i;

	i = 0;
	while (i < n) {
		if (((char *)mem1)[i] != ((char *)mem2)[i]) {
			return (absolute(((char *)mem1)[i]) - absolute(((char *)mem2)[i]));
		}
		i++;
	}
	return (0);
}
