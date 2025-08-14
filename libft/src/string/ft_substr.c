#include "libft.h"

static int	smallest(int n, int nb) {
	if (nb >= n && n > 0) {
		return (n);
	} else if (n > nb && nb > 0) {
		return (nb);
	} else {
		return (1);
	}
}

char	*ft_substr(const char *s, unsigned int start, size_t n) {
	size_t		i;
	char		*str;

	if (!s) {
		return (0);
	}
	i = 0;
	str = malloc(sizeof(char) * smallest(n + 1, ft_strlen(s) - start + 1));
	if (!str) {
		return (0);
	}
	if (start < (unsigned int)ft_strlen(s)) {
		while (i < n && s[i + start]) {
			str[i] = s[start + i];
			i++;
		}
	}
	str[i] = 0;
	return (str);
}
