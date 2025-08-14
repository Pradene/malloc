#include "libft.h"

char	*ft_strnstr(const char *s1, const char *s2, size_t n) {
	size_t	i;

	if (!ft_strlen(s2)) {
		return ((char *)s1);
	}
	if ((size_t)ft_strlen(s2) > n) {
		return (0);
	}
	i = 0;
	while (i + ft_strlen(s2) <= n && s1[i]) {
		if (ft_memcmp(s1 + i, s2, ft_strlen(s2)) == 0) {
			return ((char *)s1 + i);
		}
		i++;
	}
	return (0);
}
