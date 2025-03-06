// Written by Adrian Musceac YO8RZZ , started October 2023.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "dmridlookup.h"

DMRIdLookup::DMRIdLookup(Settings *settings, Logger *logger, QObject *parent) : QObject(parent)
{
    _settings = settings;
    _logger = logger;
    _ids = new QMap<unsigned int, QString>;
    QDir files = QDir::homePath();
    if(!QDir(files.absolutePath()+"/.config/dmrtc").exists())
    {
        QDir().mkdir(files.absolutePath()+"/.config/dmrtc");
    }
    QFileInfo dmr_id_file = files.filePath(".config/dmrtc/DMRIds.dat");
    if(!dmr_id_file.exists())
    {
        QString config = " ";
        QFile newfile(dmr_id_file.absoluteFilePath());

        if (newfile.open(QIODevice::ReadWrite))
        {
            newfile.write(config.toStdString().c_str());
            newfile.close();
        }

    }
    QFile resfile(dmr_id_file.absoluteFilePath());
    if(resfile.open(QIODevice::ReadOnly))
    {
        while (!resfile.atEnd())
        {
            QByteArray data_line = resfile.readLine();
            QString line = QString(data_line).replace("\t", ",");
            QStringList fields = line.split(",");
            if(fields.length() < 3)
                continue;
            QString data = fields[0] + " - " + fields[1] + " - " + fields[2];
            _ids->insert(fields[0].toUInt(), data);
        }
    }
}

DMRIdLookup::~DMRIdLookup()
{
    _ids->clear();
    delete _ids;
}

QString DMRIdLookup::lookup(unsigned int id)
{
    if(id == 0)
        return "0";
    if(_ids->contains(id))
    {
        QString value = _ids->value(id);
        return value.replace(",", " - ").remove("\n");
    }
    else
        return QString::number(id);
}

QString DMRIdLookup::getCallsign(unsigned int id)
{
    if(id == 0)
        return "NO CALL";
    if(_ids->contains(id))
    {
        QString value = _ids->value(id);
        QStringList fields = value.split(" - ");
        if(fields.size() > 1)
            return fields[1];
        else
            return QString::number(id);
    }
    else
        return QString::number(id);
}
