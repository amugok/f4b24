#include <string.h>
void *memset(void *buf, int ch, size_t n)
{
	char *d = buf;
	while (n--)
	{
		*d++ = ch;
	}
	return buf;
}
