#ifndef GPREDICTCONTROL_H
#define GPREDICTCONTROL_H

#include <QObject>
#include "settings.h"
#include "logger.h"

class GPredictControl : public QObject
{
    Q_OBJECT
public:
    explicit GPredictControl(const Settings *settings, Logger *logger, QObject *parent = nullptr);
    QString processMessages(QString message, int &action, qint64 &rx_freq, qint64 &tx_freq,
                            qint64 &rx_freq_delta, qint64 &tx_freq_delta);
    enum RadioAction
    {
        TuneRX,
        TuneTX,
        OffsetRX,
        OffsetTX,
        NoAction
    };

signals:

private:
    const Settings* _settings;
    Logger *_logger;
    qint64 _last_rx_frequency;
    qint64 _last_tx_frequency;

};

#endif // GPREDICTCONTROL_H
