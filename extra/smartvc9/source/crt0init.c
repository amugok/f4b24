#ifdef __cplusplus
extern "C" {
#endif

typedef int (__cdecl *_PIFV)(void);

#if _MSC_VER >= 1300
/* Visual C++ .NET(2002) or later */

#pragma section(".CRT$XCA",long,read)
#pragma section(".CRT$XCZ",long,read)
#pragma section(".CRT$XIA",long,read)
#pragma section(".CRT$XIZ",long,read)
#pragma section(".CRT$XPA",long,read)
#pragma section(".CRT$XPZ",long,read)
#pragma section(".CRT$XTA",long,read)
#pragma section(".CRT$XTZ",long,read)

#pragma comment(linker, "/merge:.CRT=.rdata")

__declspec(allocate(".CRT$XIA")) _PIFV __xi_a[] = { 0 };
__declspec(allocate(".CRT$XIZ")) _PIFV __xi_z[] = { 0 };

__declspec(allocate(".CRT$XCA")) _PIFV __xc_a[] = { 0 };
__declspec(allocate(".CRT$XCZ")) _PIFV __xc_z[] = { 0 };

__declspec(allocate(".CRT$XPA")) _PIFV __xp_a[] = { 0 };
__declspec(allocate(".CRT$XPZ")) _PIFV __xp_z[] = { 0 };

__declspec(allocate(".CRT$XTA")) _PIFV __xt_a[] = { 0 };
__declspec(allocate(".CRT$XTZ")) _PIFV __xt_z[] = { 0 };

#elif _MSC_VER >= 1200

/* Visual C++ 6.0 */

#pragma comment(linker, "/merge:.CRT=.rdata")

#pragma data_seg(".CRT$XIA")
_PIFV __xi_a[] = { 0 };
#pragma data_seg(".CRT$XIZ")
_PIFV __xi_z[] = { 0 };
#pragma data_seg(".CRT$XCA")
_PIFV __xc_a[] = { 0 };
#pragma data_seg(".CRT$XCZ")
_PIFV __xc_z[] = { 0 };
#pragma data_seg(".CRT$XPA")
_PIFV __xp_a[] = { 0 };
#pragma data_seg(".CRT$XPZ")
_PIFV __xp_z[] = { 0 };
#pragma data_seg(".CRT$XTA")
_PIFV __xt_a[] = { 0 };
#pragma data_seg(".CRT$XTZ")
_PIFV __xt_z[] = { 0 };
#pragma data_seg()

#else

#error "error:invalid VC++ version"

#endif

static int inittermsub(_PIFV * pfbegin, _PIFV * pfend, int force)
{
	int ret = 0;
	for ( ; pfbegin < pfend && (force || ret == 0); pfbegin++)
	{
		if (*pfbegin)
			ret = (**pfbegin)();
	}
	return ret;
}

void __cdecl _initterm(_PIFV * pfbegin, _PIFV * pfend)
{
	inittermsub(pfbegin, pfend, 1);
}

int __cdecl _initterm_e(_PIFV * pfbegin, _PIFV * pfend)
{
	return inittermsub(pfbegin, pfend, 0);
}

void __cdecl SmartInit()
{
	_initterm(__xi_a, __xi_z);
	_initterm(__xc_a, __xc_z);
}

void __cdecl SmartTerm()
{
	_initterm(__xp_a, __xp_z);
	_initterm(__xt_a, __xt_z);
}

#ifdef __cplusplus
}
#endif
