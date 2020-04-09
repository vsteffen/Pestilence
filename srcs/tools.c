#include "famine.h"


int	ft_strcmp(const char *s1, const char *s2)
{
	while (*s1)
	{
		if (*s1 != *s2)
			return (((unsigned char)*s1 - (unsigned char)*s2));
		s1++;
		s2++;
	}
	return (((unsigned char)*s1 - (unsigned char)*s2));
}

void	*ft_memchr(const void *s, int c, size_t n)
{
	const unsigned char *tmp_s = s;

	while (n--)
	{
		if (*tmp_s == (unsigned char)c)
			return ((void*)tmp_s);
		tmp_s++;
	}
	return (NULL);
}

void	*ft_memcpy(void *dest, const void *src, size_t n)
{
	size_t pos;

	pos = 0;
	while (pos < n)
	{
		((char*)dest)[pos] = ((char*)src)[pos];
		pos++;
	}
	return (dest);
}

void	*ft_memset(void *s, int c, size_t n)
{
	size_t pos;

	pos = 0;
	while (pos < n)
	{
		((char*)s)[pos] = (char)c;
		pos++;
	}
	return (s);
}
