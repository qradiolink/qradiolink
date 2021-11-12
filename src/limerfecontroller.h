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


signals:

public slots:
    void init();
    void deinit();

private:
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

};

#endif // LIMERFECONTROLLER_H
