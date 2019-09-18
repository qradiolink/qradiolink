#ifndef RADIOCHANNEL_H
#define RADIOCHANNEL_H

#include <QObject>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <libconfig.h++>
#include <iostream>

class RadioChannel : public QObject
{
    Q_OBJECT
public:
    explicit RadioChannel(QObject *parent = 0);
    QFileInfo *setupConfig();
    void readConfig();
    void saveConfig();

signals:

public slots:

private:
    QFileInfo *_memories_file;

};

#endif // RADIOCHANNEL_H
