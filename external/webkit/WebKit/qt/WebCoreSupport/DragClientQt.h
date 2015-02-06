

#include "DragClient.h"
class QWebPage;
namespace WebCore {

class DragClientQt : public DragClient {
public:
    DragClientQt(QWebPage* webPage) : m_webPage(webPage) {};
    virtual void willPerformDragDestinationAction(DragDestinationAction,
                                                  DragData*);
    virtual WebCore::DragDestinationAction actionMaskForDrag(DragData*);
    virtual void dragControllerDestroyed();
    virtual DragSourceAction dragSourceActionMaskForPoint(const IntPoint&);
    virtual void willPerformDragSourceAction(DragSourceAction, const IntPoint&, Clipboard*);    
    virtual void startDrag(DragImageRef dragImage, const IntPoint& dragImageOrigin, const IntPoint& eventPos, Clipboard*, Frame*, bool linkDrag = false);
    virtual DragImageRef createDragImageForLink(KURL&, const String& label, Frame*);
private:
    QWebPage* m_webPage;
};

}

