#ifndef CHANNEL_H
#define CHANNEL_H

#include <QObject>

class Channel : public QObject
{
    Q_OBJECT
public:
    explicit Channel(int id, int parent_id, QString name, QString description, QObject *parent = 0);
    int id;
    int parent_id;
    int position;
    bool temporary;
    QString name;
    QString description;

signals:

public slots:

};

#endif // CHANNEL_H
