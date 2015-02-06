

#ifndef QWEBHISTORY_H
#define QWEBHISTORY_H

#include <QtCore/qurl.h>
#include <QtCore/qstring.h>
#include <QtGui/qicon.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qshareddata.h>

#include "qwebkitglobal.h"

class QWebPage;

namespace WebCore {
    class FrameLoaderClientQt;
}

class QWebHistoryItemPrivate;

class QWEBKIT_EXPORT QWebHistoryItem {
public:
    QWebHistoryItem(const QWebHistoryItem &other);
    QWebHistoryItem &operator=(const QWebHistoryItem &other);
    ~QWebHistoryItem();

    QUrl originalUrl() const;
    QUrl url() const;

    QString title() const;
    QDateTime lastVisited() const;

    QIcon icon() const;

    QVariant userData() const;
    void setUserData(const QVariant& userData);

    bool isValid() const;

private:
    QWebHistoryItem(QWebHistoryItemPrivate *priv);
    friend class QWebHistory;
    friend class QWebPage;
    friend class WebCore::FrameLoaderClientQt;
    friend class QWebHistoryItemPrivate;
    //friend QDataStream & operator<<(QDataStream& out,const QWebHistoryItem& hist);
    //friend QDataStream & operator>>(QDataStream& in,QWebHistoryItem& hist);
    QExplicitlySharedDataPointer<QWebHistoryItemPrivate> d;
};


class QWebHistoryPrivate;
class QWEBKIT_EXPORT QWebHistory {
public:
    void clear();

    QList<QWebHistoryItem> items() const;
    QList<QWebHistoryItem> backItems(int maxItems) const;
    QList<QWebHistoryItem> forwardItems(int maxItems) const;

    bool canGoBack() const;
    bool canGoForward() const;

    void back();
    void forward();
    void goToItem(const QWebHistoryItem &item);

    QWebHistoryItem backItem() const;
    QWebHistoryItem currentItem() const;
    QWebHistoryItem forwardItem() const;
    QWebHistoryItem itemAt(int i) const;

    int currentItemIndex() const;

    int count() const;

    int maximumItemCount() const;
    void setMaximumItemCount(int count);

private:
    QWebHistory();
    ~QWebHistory();

    friend class QWebPage;
    friend class QWebPagePrivate;
    friend QWEBKIT_EXPORT QDataStream& operator>>(QDataStream&, QWebHistory&);
    friend QWEBKIT_EXPORT QDataStream& operator<<(QDataStream&, const QWebHistory&);

    Q_DISABLE_COPY(QWebHistory)

    QWebHistoryPrivate *d;
};

QWEBKIT_EXPORT QDataStream& operator<<(QDataStream& stream, const QWebHistory& history);
QWEBKIT_EXPORT QDataStream& operator>>(QDataStream& stream, QWebHistory& history);

#endif
