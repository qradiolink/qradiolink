// Copyright (c) 2014-2021 Josh Blum
// SPDX-License-Identifier: BSL-1.0
// Copyright Lime Microsystems Apache 2.0 License

#include <SoapySDR/Device.hpp>
#include <lime/ConnectionRegistry.h>
#include <iostream>
#include <algorithm>
#include <QDebug>
#include "devices.h"


int findSoapyDevices(const std::string &argStr, const bool sparse, QList<QString> &devices)
{
    const auto results = SoapySDR::Device::enumerate(argStr);
    if (sparse)
    {
        std::vector<std::string> sparseResults;
        for (size_t i = 0; i < results.size(); i++)
        {
            const auto it = results[i].find("label");
            if (it != results[i].end()) sparseResults.push_back(it->second);
            else sparseResults.push_back(SoapySDR::KwargsToString(results[i]));
        }
        std::sort(sparseResults.begin(), sparseResults.end());
        for (size_t i = 0; i < sparseResults.size(); i++)
        {
            QString dev_str = QString("soapy=%1,").arg(i) + QString::fromStdString(
                        SoapySDR::KwargsToString(results[i])).replace(" ", "");
            devices.push_back(dev_str);
        }
    }
    else
    {
        for (size_t i = 0; i < results.size(); i++)
        {
            std::cout << "Found device " << i << std::endl;
            for (const auto &it : results[i])
            {
                std::cout << "  " << it.first << " = " << it.second << std::endl;
            }
            std::cout << std::endl;
        }
        if (results.empty()) std::cerr << "No devices found! " << argStr << std::endl;
        else std::cout << std::endl;
    }
    return results.empty()?EXIT_FAILURE:EXIT_SUCCESS;
}

void findLimeDevices(QList<QString> &devices)
{
    std::string argStr;

    lime::ConnectionHandle hint(argStr);

    auto handles = lime::ConnectionRegistry::findConnections(hint);
    for (const auto &handle : handles)
    {
        //std::cout << "  * [" << handle.serialize() << "]" << std::endl;
        QString dev_str = QString("name=%1,driver=lime,serial=%2").arg(
                    QString::fromStdString(handle.name)).arg(QString::fromStdString(handle.serial));
        devices.push_back(dev_str);
    }

    //std::cout << std::endl;
}
