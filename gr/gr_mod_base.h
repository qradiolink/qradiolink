#ifndef GR_MOD_BASE_H
#define GR_MOD_BASE_H

#include <QObject>

class gr_mod_base : public QObject
{
    Q_OBJECT
public:
    explicit gr_mod_base(QObject *parent = 0);

signals:

public slots:

};

#endif // GR_MOD_BASE_H
