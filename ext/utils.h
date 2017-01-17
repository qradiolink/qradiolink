#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include <QString>
#include <QtEndian>
#include <string>
#include <sys/time.h>
#include "config_defines.h"

void genRandomStr(char *str, const int len);
void addPreamble(quint8 *buffer, quint16 type, quint32 len);
void getPreamble(quint8 *buffer, int *type, int *len);



#endif // UTILS_H
