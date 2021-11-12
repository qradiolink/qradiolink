#ifndef LIMERFECONTROLLER_H
#define LIMERFECONTROLLER_H

#include <QObject>
#include <lime/limeRFE.h>
#include "settings.h"

class LimeRFEController : public QObject
{
    Q_OBJECT
public:
    explicit LimeRFEController(const Settings *settings, Logger *logger, QObject *parent = nullptr);
    ~LimeRFEController();


signals:

public slots:
    void init();
    void deinit();

private:
    Logger *_logger;
    const Settings *_settings;
    bool _lime_rfe_inited;
    rfe_dev_t *_lime_rfe;

};

#endif // LIMERFECONTROLLER_H
