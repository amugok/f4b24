#include <string.h>

void *memove(void *dest, const void *src, size_t n)
{
	char *d = (char *)dest;
	const char *s = (const char *)src;

	if (s == d || n == 0)
	{
		return dest;
	}

	if (s < d)
	{
		s = s + n - 1;
		d = d + n - 1;
		while (n--)
		{
			*d-- = *s--;
		}
	}
	else
	{
		while (n--)
		{
			*d++ = *s++;
		}
	}
	return dest;
}
