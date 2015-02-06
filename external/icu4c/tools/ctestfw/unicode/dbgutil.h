


/** C++ Utilities to aid in debugging **/

#ifndef _DBGUTIL_H
#define _DBGUTIL_H

#include "unicode/utypes.h"
#include "unicode/udbgutil.h"
#include "unicode/unistr.h"
#include "unicode/testtype.h"

#if !UCONFIG_NO_FORMATTING

//class UnicodeString;

T_CTEST_API const UnicodeString&  T_CTEST_EXPORT2 udbg_enumString(UDebugEnumType type, int32_t field);

T_CTEST_API int32_t  T_CTEST_EXPORT2 udbg_enumByString(UDebugEnumType type, const UnicodeString& string);

T_CTEST_API int32_t T_CTEST_EXPORT2 udbg_stoi(const UnicodeString &s);

T_CTEST_API double T_CTEST_EXPORT2 udbg_stod(const UnicodeString &s);

T_CTEST_API UnicodeString *udbg_escape(const UnicodeString &s, UnicodeString *dst);

#endif

#endif
