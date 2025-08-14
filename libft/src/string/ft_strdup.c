#include "libft.h"

char	*ft_strdup(const char *s) {
	char	*str;

	str = malloc(sizeof(char) * (ft_strlen(s) + 1));
	if (!str) {
		return (0);
	}
	str = ft_memcpy(str, s, ft_strlen(s) + 1);
	return (str);
}
