

#ifndef FloatSize_h
#define FloatSize_h

#include "IntSize.h"
#include <wtf/MathExtras.h>
#include <wtf/Platform.h>

#if PLATFORM(CG)
typedef struct CGSize CGSize;
#endif

#if PLATFORM(MAC) || (PLATFORM(CHROMIUM) && OS(DARWIN))
#ifdef NSGEOMETRY_TYPES_SAME_AS_CGGEOMETRY_TYPES
typedef struct CGSize NSSize;
#else
typedef struct _NSSize NSSize;
#endif
#endif

namespace WebCore {

class IntSize;

class FloatSize {
public:
    FloatSize() : m_width(0), m_height(0) { }
    FloatSize(float width, float height) : m_width(width), m_height(height) { }
    FloatSize(const IntSize&);

    static FloatSize narrowPrecision(double width, double height);

    float width() const { return m_width; }
    float height() const { return m_height; }

    void setWidth(float width) { m_width = width; }
    void setHeight(float height) { m_height = height; }

    bool isEmpty() const { return m_width <= 0 || m_height <= 0; }

    FloatSize expandedTo(const FloatSize& other) const
    {
        return FloatSize(m_width > other.m_width ? m_width : other.m_width,
            m_height > other.m_height ? m_height : other.m_height);
    }

    FloatSize shrunkTo(const FloatSize& other) const
    {
       return FloatSize(m_width < other.m_width ? m_width : other.m_width,
           m_height < other.m_height ? m_height : other.m_height);
    }

#if PLATFORM(CG)
    explicit FloatSize(const CGSize&); // don't do this implicitly since it's lossy
    operator CGSize() const;
#endif

#if (PLATFORM(MAC) && !defined(NSGEOMETRY_TYPES_SAME_AS_CGGEOMETRY_TYPES)) \
        || (PLATFORM(CHROMIUM) && OS(DARWIN))
    explicit FloatSize(const NSSize &); // don't do this implicitly since it's lossy
    operator NSSize() const;
#endif

private:
    float m_width, m_height;
};

inline FloatSize& operator+=(FloatSize& a, const FloatSize& b)
{
    a.setWidth(a.width() + b.width());
    a.setHeight(a.height() + b.height());
    return a;
}

inline FloatSize& operator-=(FloatSize& a, const FloatSize& b)
{
    a.setWidth(a.width() - b.width());
    a.setHeight(a.height() - b.height());
    return a;
}

inline FloatSize operator+(const FloatSize& a, const FloatSize& b)
{
    return FloatSize(a.width() + b.width(), a.height() + b.height());
}

inline FloatSize operator-(const FloatSize& a, const FloatSize& b)
{
    return FloatSize(a.width() - b.width(), a.height() - b.height());
}

inline FloatSize operator-(const FloatSize& size)
{
    return FloatSize(-size.width(), -size.height());
}

inline bool operator==(const FloatSize& a, const FloatSize& b)
{
    return a.width() == b.width() && a.height() == b.height();
}

inline bool operator!=(const FloatSize& a, const FloatSize& b)
{
    return a.width() != b.width() || a.height() != b.height();
}

inline IntSize roundedIntSize(const FloatSize& p)
{
    return IntSize(static_cast<int>(roundf(p.width())), static_cast<int>(roundf(p.height())));
}

} // namespace WebCore

#endif // FloatSize_h
