

#ifndef CPPUTILS_H
#define CPPUTILS_H

#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "cmemory.h"

/*==========================================================================*/
/* Array copy utility functions */
/*==========================================================================*/

static
inline void uprv_arrayCopy(const double* src, double* dst, int32_t count)
{ uprv_memcpy(dst, src, (size_t)(count * sizeof(*src))); }

static
inline void uprv_arrayCopy(const double* src, int32_t srcStart,
              double* dst, int32_t dstStart, int32_t count)
{ uprv_memcpy(dst+dstStart, src+srcStart, (size_t)(count * sizeof(*src))); }

static
inline void uprv_arrayCopy(const int8_t* src, int8_t* dst, int32_t count)
    { uprv_memcpy(dst, src, (size_t)(count * sizeof(*src))); }

static
inline void uprv_arrayCopy(const int8_t* src, int32_t srcStart,
              int8_t* dst, int32_t dstStart, int32_t count)
{ uprv_memcpy(dst+dstStart, src+srcStart, (size_t)(count * sizeof(*src))); }

static
inline void uprv_arrayCopy(const int16_t* src, int16_t* dst, int32_t count)
{ uprv_memcpy(dst, src, (size_t)(count * sizeof(*src))); }

static
inline void uprv_arrayCopy(const int16_t* src, int32_t srcStart,
              int16_t* dst, int32_t dstStart, int32_t count)
{ uprv_memcpy(dst+dstStart, src+srcStart, (size_t)(count * sizeof(*src))); }

static
inline void uprv_arrayCopy(const int32_t* src, int32_t* dst, int32_t count)
{ uprv_memcpy(dst, src, (size_t)(count * sizeof(*src))); }

static
inline void uprv_arrayCopy(const int32_t* src, int32_t srcStart,
              int32_t* dst, int32_t dstStart, int32_t count)
{ uprv_memcpy(dst+dstStart, src+srcStart, (size_t)(count * sizeof(*src))); }

static
inline void
uprv_arrayCopy(const UChar *src, int32_t srcStart,
        UChar *dst, int32_t dstStart, int32_t count)
{ uprv_memcpy(dst+dstStart, src+srcStart, (size_t)(count * sizeof(*src))); }

static inline void
uprv_arrayCopy(const U_NAMESPACE_QUALIFIER UnicodeString *src, U_NAMESPACE_QUALIFIER UnicodeString *dst, int32_t count)
{ while(count-- > 0) *dst++ = *src++; }

static inline void
uprv_arrayCopy(const U_NAMESPACE_QUALIFIER UnicodeString *src, int32_t srcStart,
        U_NAMESPACE_QUALIFIER UnicodeString *dst, int32_t dstStart, int32_t count)
{ uprv_arrayCopy(src+srcStart, dst+dstStart, count); }

inline void
uprv_checkCanGetBuffer(const U_NAMESPACE_QUALIFIER UnicodeString &s, UErrorCode &errorCode) {
    if(U_SUCCESS(errorCode) && s.isBogus()) {
        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
    }
}

#endif /* _CPPUTILS */
