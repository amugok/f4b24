#include <string.h>

void *memcpy(void *buf1, const void *buf2, size_t n)
{
	char *d = buf1;
	const char *s = buf2;
	while (n--)
	{
		*d++ = *s++;
	}
	return buf1;
}
