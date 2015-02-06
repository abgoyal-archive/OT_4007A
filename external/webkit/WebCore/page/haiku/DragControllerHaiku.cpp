

#include "config.h"
#include "DragController.h"

#include "DragData.h"

#include <InterfaceDefs.h>


namespace WebCore {

// FIXME: These values are straight out of DragControllerMac, so probably have
// little correlation with Haiku standards...
const int DragController::LinkDragBorderInset = 2;
const int DragController::MaxOriginalImageArea = 1500 * 1500;
const int DragController::DragIconRightInset = 7;
const int DragController::DragIconBottomInset = 3;

const float DragController::DragImageAlpha = 0.75f;


bool DragController::isCopyKeyDown()
{
    if (modifiers() & B_COMMAND_KEY)
        return true;

    return false;
}

DragOperation DragController::dragOperation(DragData* dragData)
{
    // FIXME: This logic is incomplete
    if (dragData->containsURL())
        return DragOperationCopy;

    return DragOperationNone;
}

const IntSize& DragController::maxDragImageSize()
{
    static const IntSize maxDragImageSize(400, 400);

    return maxDragImageSize;
}

void DragController::cleanupAfterSystemDrag()
{
}

} // namespace WebCore

