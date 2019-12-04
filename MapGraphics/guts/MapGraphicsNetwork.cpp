#include "MapGraphicsNetwork.h"

#include <QMutexLocker>
#include <QNetworkRequest>
#include <QThread>
#include <QtDebug>

const QByteArray DEFAULT_USER_AGENT = "MapGraphics";

//static
QHash<QThread *, MapGraphicsNetwork *> MapGraphicsNetwork::_instances = QHash<QThread *, MapGraphicsNetwork*>();
QMutex MapGraphicsNetwork::_mutex;

//static
MapGraphicsNetwork *MapGraphicsNetwork::getInstance()
{
    QMutexLocker lock(&_mutex);
    QThread * current = QThread::currentThread();
    if (!MapGraphicsNetwork::_instances.contains(current))
        MapGraphicsNetwork::_instances.insert(current, new MapGraphicsNetwork());
    return MapGraphicsNetwork::_instances.value(current);
}

MapGraphicsNetwork::~MapGraphicsNetwork()
{
    delete _manager;
    _manager = 0;
}

QNetworkReply *MapGraphicsNetwork::get(QNetworkRequest &request)
{
    request.setRawHeader("User-Agent",
                         _userAgent);
    QNetworkReply * toRet = _manager->get(request);
    return toRet;
}

void MapGraphicsNetwork::setUserAgent(const QByteArray &agent)
{
    _userAgent = agent;
}

QByteArray MapGraphicsNetwork::userAgent() const
{
    return _userAgent;
}

//protected
MapGraphicsNetwork::MapGraphicsNetwork()
{
    _manager = new QNetworkAccessManager();
    this->setUserAgent(DEFAULT_USER_AGENT);
}
