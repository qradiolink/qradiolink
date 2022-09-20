#ifndef DEVICES_H
#define DEVICES_H
#include <QList>
#include <string>

void findLimeDevices(QList<QString> &devices);
int findSoapyDevices(const std::string &argStr, const bool sparse, QList<QString> &devices);

#endif // DEVICES_H
