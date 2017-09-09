#ifndef GR_DEMOD_BASE_H
#define GR_DEMOD_BASE_H

#include <QObject>

class gr_demod_base : public QObject
{
    Q_OBJECT
public:
    explicit gr_demod_base(QObject *parent = 0);

signals:

public slots:

};

#endif // GR_DEMOD_BASE_H
