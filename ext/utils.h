#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>
#include <QtEndian>
#include <string>
#include <vector>
#include <complex>
#include <sys/time.h>
#include "config_defines.h"

static const float tone_list[]= {67.0, 71.9, 74.4, 77.0, 79.7, 82.5, 85.4, 88.5, 81.5, 87.4, 94.8, 100.0, 103.5, 107.2, 110.9,
                     114.8, 118.8, 123.0, 127.3, 131.8, 136.5, 141.3, 146.2, 151.4, 156.7, 162.2,
                      167.9, 173.8, 179.9, 186.2, 192.8, 203.5, 210.7, 218.1, 225.7, 233.6, 241.8, 250.3};

void genRandomStr(char *str, const int len);
void addPreamble(quint8 *buffer, quint16 type, quint32 len);
void getPreamble(quint8 *buffer, int *type, int *len);
void buildFilterWidthList(std::vector<std::complex<int> > *filter_widths,
                          std::vector<std::complex<int> > *ranges, std::vector<bool> *symmetric);
void buildModeList(QVector<QString> *operating_modes);
void unpackBytes(unsigned char *bitbuf, const unsigned char *bytebuf, int bytecount);

#endif // UTILS_H
