#include <string.h>

int memcmp(const void *buf1, const void *buf2, size_t n)
{
	const unsigned char *s1 = buf1;
	const unsigned char *s2 = buf2;
	while ( n-- )
	{
		unsigned char u1 = *s1++;
		unsigned char u2 = *s2++;
		if ( u1 != u2 )
		{
			return u1 - u2;
		}
	}
	return 0;
}
