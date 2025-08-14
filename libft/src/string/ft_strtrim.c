#include "libft.h"

static int	start(char const *s1, char const *set) {
	int	i;

	i = 0;
	while (s1[i]) {
		if (ft_strchr(set, s1[i]) == 0) {
			break ;
		}
		i++;
	}
	return (i);
}

static int	end(char const *s1, char const *set) {
	int	i;

	i = ft_strlen(s1) - 1;
	while (i) {
		if (ft_strrchr(set, s1[i]) == 0) {
			break ;
		}
		i--;
	}
	return (i);
}

char	*ft_strtrim(char const *s1, char const *set) {
	char	*str;
	int		s;
	int		e;
	int		i;

	if (!s1) {
		return (0);
	}
	if (!set) {
		return (ft_strdup(s1));
	}
	s = start(s1, set);
	e = end(s1, set);
	if (s > e) {
		return (ft_strdup(""));
	}
	str = ft_calloc(e - s + 1 + 1, sizeof(char));
	if (!str) {
		return (0);
	}
	i = 0;
	while (i < e - s + 1) {
		str[i] = s1[s + i];
		++i;
	}
	return (str);
}
