
#define XLOG_TAG "WebCore/webkit_plugin"

#include "config.h"
#include "PluginTimer.h"
#include <utils/CallStack.h> 

#include <cutils/xlog.h>

namespace WebCore {

    static uint32 gTimerID;

    PluginTimer::PluginTimer(PluginTimer** list, NPP instance, bool repeat,
                             void (*timerFunc)(NPP npp, uint32 timerID))
                : m_list(list),
                  m_instance(instance),
                  m_timerFunc(timerFunc),
                  m_repeat(repeat),
                  m_unscheduled(false)
    {
        m_timerID = ++gTimerID;

        m_next = *list;
        if (m_next) {
            m_next->m_prev = this;
        }
        m_prev = 0;
        *list = this;
    }
    
    PluginTimer::~PluginTimer()
    {
        if (m_next) {
            m_next->m_prev = m_prev;
        }
        if (m_prev) {
            m_prev->m_next = m_next;
        } else {
            if (m_list != NULL) {
                *m_list = m_next;
            }
        }
    }
        
    void PluginTimer::fired()
    {
        if (!m_unscheduled)
            m_timerFunc(m_instance, m_timerID);

        if (!m_repeat || m_unscheduled) {
        	XLOGD2("*****PluginTimer::fired, delete this=%p", this);
            delete this;
        }
    }
    
    // may return null if timerID is not found
    PluginTimer* PluginTimer::Find(PluginTimer* list, uint32 timerID)
    {
    	XLOGD2("*****PluginTimer::Find, list=%p, timerID=%u", list, timerID);
        PluginTimer* curr = list;
        while (curr) {
            if (curr->m_timerID == timerID) {
                break;
            }
            curr = curr->m_next;
        }
        return curr;
    }

    ///////////////////////////////////////////////////////////////////////////
    
    PluginTimerList::~PluginTimerList()
    {
    	XLOGD2("*****PluginTimerList::~PluginTimerList, m_list=%p, this=%p", m_list, this);
        while (m_list) {
            if (!m_list->m_unscheduled) {
                m_list->m_timerFunc = NULL;
                m_list->m_unscheduled = true;
                m_list->m_list = NULL;

                m_list->m_prev = NULL;
                if (m_list->m_next) {
                    m_list->m_next->m_prev = NULL;
                }

                PluginTimer *m = m_list->m_next;
                m_list->m_next = NULL;
                m_list = m;
            }
            else {
                delete m_list;
            }
        }
    }

    uint32 PluginTimerList::schedule(NPP instance, uint32 interval, bool repeat,
                                     void (*proc)(NPP npp, uint32 timerID))
    {        
        PluginTimer* timer = new PluginTimer(&m_list, instance, repeat, proc);
        XLOGD2("*****PluginTimerList::schedule, new PluginTimer timer=%p", timer);
        
        double dinterval = interval * 0.001;    // milliseconds to seconds
        if (repeat) {
            timer->startRepeating(dinterval);
        } else {
            timer->startOneShot(dinterval);
        }
        return timer->timerID();
    }
    
    void PluginTimerList::unschedule(NPP instance, uint32 timerID)
    {
        // Although it looks like simply deleting the timer would work here
        // (stop() will be executed by the dtor), we cannot do this, as
        // the plugin can call us while we are in the fired() method,
        // (when we execute the timerFunc callback). Deleting the object
        // we are in would then be a rather bad move...
        PluginTimer* timer = PluginTimer::Find(m_list, timerID);
        XLOGD2("*****PluginTimerList::unschedule, find timer=%p", timer);
        if (timer)
            timer->unschedule();
    }
    
} // namespace WebCore
