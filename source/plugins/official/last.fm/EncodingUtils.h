#ifndef ENCODINGUTILS_H
#define ENCODINGUTILS_H

#define USE_LOGGER (0)

#if USE_LOGGER
    #include "Logger.h"
#else
    #include <windows.h>
    #define LOG(x, y)
#endif

#include <string>

/*************************************************************************/ /**
    A bunch of static functions for converting between different character
    sets.
******************************************************************************/
class EncodingUtils
{
public:

    /*********************************************************************/ /**
        Converts an ANSI string to a UTF-8 string.
        
        @param[in] ansi
    **************************************************************************/
    static int
    AnsiToUtf8(
        const char* ansi,
        char*       utf8,
        int         nUtf8Size);
        
    static int
    UnicodeToUtf8(
        const WCHAR* lpWideCharStr,
        int          cwcChars,
        char*        lpUtf8Str,
        int          nUtf8Size);

    static std::string
    Utf8ToAnsi(
        const char* pcUTF8Str);

	
private:


};

#endif // ENCODINGUTILS_H