#ifndef RADIOCHANNEL_H
#define RADIOCHANNEL_H

#include <QObject>
#include <QVector>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <libconfig.h++>
#include <iostream>
#include <string>

struct radiochannel
{
    radiochannel() : id(0), rx_frequency(0), tx_frequency(0), tx_shift(0), rx_mode(0), tx_mode(0), name("") {}
    int id;
    long long rx_frequency;
    long long tx_frequency;
    long tx_shift;
    int rx_mode;
    int tx_mode;
    std::string name;
};

class RadioChannels : public QObject
{
    Q_OBJECT
public:
    explicit RadioChannels(QObject *parent = 0);
    ~RadioChannels();
    QFileInfo *setupConfig();
    void readConfig();
    void saveConfig();
    QVector<radiochannel>* getChannels();

signals:

public slots:

private:
    QFileInfo *_memories_file;
    QVector<radiochannel> *_channels;

};

#endif // RADIOCHANNEL_H
