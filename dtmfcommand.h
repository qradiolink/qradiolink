#ifndef DTMFCOMMAND_H
#define DTMFCOMMAND_H

#include <QObject>
#include <QString>
#include "speech.h"
#include "databaseapi.h"
#include "station.h"
#include "config_defines.h"
#include "mumbleclient.h"
#include "ext/dec.h"
#include "settings.h"

class DtmfCommand : public QObject
{
    Q_OBJECT
public:
    explicit DtmfCommand(Settings *settings, DatabaseApi *db, MumbleClient *mumble, QObject *parent = 0);
    ~DtmfCommand();
signals:
    void speak(QString);
    void readyInput();
    void tellStations();
public slots:
    void haveCall(QVector<char> *dtmf);
    void haveCommand(QVector<char> *dtmf);
    void channelReady(int chan_number);
    void newStation(Station *s);
    void leftStation(Station *s);

private:
    DatabaseApi *_db;
    int _in_conference;
    int _id;
    int _conference_id;
    QVector<Station*> *_conference_stations;
    std::string _dialing_number;

    Station *_current_station;
    MumbleClient *_mumble;
    Settings *_settings;
};

#endif // DTMFCOMMAND_H
