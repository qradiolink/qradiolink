#ifndef MAPGRAPHICSNETWORK_H
#define MAPGRAPHICSNETWORK_H

#include <QMutex>
#include <QNetworkAccessManager>
#include <QHash>

class MapGraphicsNetwork
{
public:
    static MapGraphicsNetwork * getInstance();

    ~MapGraphicsNetwork();

    QNetworkReply * get(QNetworkRequest& request);

    void setUserAgent(const QByteArray& agent);
    QByteArray userAgent() const;

protected:
    MapGraphicsNetwork();

private:
    static QHash<QThread *, MapGraphicsNetwork *> _instances;
    static QMutex _mutex;
    QNetworkAccessManager * _manager;

    QByteArray _userAgent;
};

#endif // MAPGRAPHICSNETWORK_H
