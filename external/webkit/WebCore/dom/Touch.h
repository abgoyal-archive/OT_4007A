

#ifndef TOUCH_H_
#define TOUCH_H_

#if ENABLE(TOUCH_EVENTS)

#include "EventTarget.h"
#include "Frame.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Touch : public RefCounted<Touch> {
public:
    static PassRefPtr<Touch> create(Frame* frame, EventTarget* target,
            unsigned identifier, int screenX, int screenY, int pageX, int pageY)
    {
        return adoptRef(new Touch(frame, target, identifier, screenX, 
                screenY, pageX, pageY));
    }

    EventTarget* target() const { return m_target.get(); }
    unsigned identifier() const { return m_identifier; }
    int clientX() const { return m_clientX; }
    int clientY() const { return m_clientY; }
    int screenX() const { return m_screenX; }
    int screenY() const { return m_screenY; }
    int pageX() const { return m_pageX; }
    int pageY() const { return m_pageY; }

private:
    Touch(Frame* frame, EventTarget* target, unsigned identifier,
            int screenX, int screenY, int pageX, int pageY);

    RefPtr<EventTarget> m_target;
    unsigned m_identifier;
    int m_clientX;
    int m_clientY;
    int m_screenX;
    int m_screenY;
    int m_pageX;
    int m_pageY;
};

} // namespace WebCore

#endif // ENABLE(TOUCH_EVENTS)

#endif /* TOUCH_H_ */
