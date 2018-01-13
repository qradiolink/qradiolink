#ifndef RADIOPROTOCOL_H
#define RADIOPROTOCOL_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QXmlStreamWriter>
#include "channel.h"
#include "ext/QRadioLink.pb.h"

class RadioProtocol : public QObject
{
    Q_OBJECT
public:
    explicit RadioProtocol(QObject *parent = 0);

    QByteArray buildChannelList();
    void addChannel(Channel *chan);

signals:

public slots:

private:
    QVector<Channel*> *_voip_channels;

};

#endif // RADIOPROTOCOL_H
