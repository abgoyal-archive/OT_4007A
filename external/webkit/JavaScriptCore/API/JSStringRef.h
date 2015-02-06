

#ifndef JSStringRef_h
#define JSStringRef_h

#include <JavaScriptCore/JSValueRef.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <stddef.h> /* for size_t */

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(WIN32) && !defined(_WIN32) && !defined(__WINSCW__) \
    && !(defined(__CC_ARM) || defined(__ARMCC__)) /* RVCT */
    typedef unsigned short JSChar;
#else
    typedef wchar_t JSChar;
#endif

JS_EXPORT JSStringRef JSStringCreateWithCharacters(const JSChar* chars, size_t numChars);
JS_EXPORT JSStringRef JSStringCreateWithUTF8CString(const char* string);

JS_EXPORT JSStringRef JSStringRetain(JSStringRef string);
JS_EXPORT void JSStringRelease(JSStringRef string);

JS_EXPORT size_t JSStringGetLength(JSStringRef string);
JS_EXPORT const JSChar* JSStringGetCharactersPtr(JSStringRef string);

JS_EXPORT size_t JSStringGetMaximumUTF8CStringSize(JSStringRef string);
JS_EXPORT size_t JSStringGetUTF8CString(JSStringRef string, char* buffer, size_t bufferSize);

JS_EXPORT bool JSStringIsEqual(JSStringRef a, JSStringRef b);
JS_EXPORT bool JSStringIsEqualToUTF8CString(JSStringRef a, const char* b);

#ifdef __cplusplus
}
#endif

#endif /* JSStringRef_h */
