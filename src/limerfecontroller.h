#ifndef LIMERFECONTROLLER_H
#define LIMERFECONTROLLER_H

#include <QObject>
#include <lime/limeRFE.h>
#include "settings.h"
#include "limits.h"

class LimeRFEController : public QObject
{
    Q_OBJECT
public:
    explicit LimeRFEController(const Settings *settings, Logger *logger, QObject *parent = nullptr);
    ~LimeRFEController();
    void setRXBand(int64_t rx_frequency);
    void setTXBand(int64_t tx_frequency);
    void setDuplex(bool duplex_mode);
    void setTransmit(bool tx_on);
    void setAttenuator(int value);
    void setNotchFilter(bool enable);


signals:

public slots:
    void init();
    void deinit();

private:
    QString getError(int res);
    Logger *_logger;
    Limits *_limits;
    const Settings *_settings;
    bool _lime_rfe_inited;
    rfe_dev_t *_lime_rfe;
    rfe_boardState _board_state;
    bool _duplex_mode;
    bool _transmit_on;
    int _current_rx_band;
    int _current_tx_band;
    bool _low_band_tx; // used only for TX port when switching RF paths in duplex mode

};

#endif // LIMERFECONTROLLER_H
