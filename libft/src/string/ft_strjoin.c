#include "libft.h"

char	*ft_strjoin(char const *s1, char const *s2) {
	int		i;
	int		size;
	char	*str;

	i = 0;
	size = ft_strlen(s1);
	str = malloc((ft_strlen(s1) + ft_strlen(s2) + 1) * sizeof(char));
	if (!str) {
		return (0);
	}
	str = ft_memcpy(str, s1, ft_strlen(s1));
	while (i < ft_strlen(s2)) {
		str[i + size] = s2[i];
		i++;
	}
	str[i + size] = 0;
	return (str);
}
