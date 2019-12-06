// Written by Adrian Musceac YO8RZZ at gmail dot com, started August 2013.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
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

#include "aprs.h"

Aprs::Aprs(const Settings *settings)
{
    _settings = settings;
    _socket = new QTcpSocket;
    QObject::connect(_socket,SIGNAL(error(QAbstractSocket::SocketError )),this,SLOT(connectionFailed(QAbstractSocket::SocketError)));
    QObject::connect(_socket,SIGNAL(connected()),this,SLOT(connectionSuccess()));
    QObject::connect(_socket,SIGNAL(readyRead()),this,SLOT(processData()));
    _connection_tries=0;
    _status=0;
    _authenticated = 0;
    QString aprs_server;
#ifdef TEST_APRS
     _hostname = settings->aprs_server;
     _aprs_settings = settings->aprs_settings;
     _init_latitude = settings->aprs_init_latitude;
     _init_longitude = settings->aprs_init_longitude;
     _plot_range = settings->aprs_plot_range;
#else

    _aprs_settings = "XASTIR";
    _init_latitude = 46.0;
    _init_longitude = 26.0;
    _plot_range = 200;
    _hostname = "rotate.aprs.net";

#endif
    _delaytime = QTime::currentTime();
    this->connectToAPRS();
}


Aprs::~Aprs()
{
    _socket->disconnectFromHost();
    delete _socket;
}


void Aprs::connectionSuccess()
{
    qDebug("Connection to APRS established.");

    _status=1;
    _connection_tries=0;

    this->authenticate();

    emit connectedToAPRS();
}


void Aprs::connectionFailed(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    _status=0;
    qDebug("OOps! Could not connect to APRS. Trying again.");
    _connection_tries++;
    if(_connection_tries < 30)
    {
        this->connectToAPRS();
    }
    else
    {
        qDebug("Giving up! APRS is not reachable.");
        emit connectionFailure();
    }

}


void Aprs::connectToAPRS()
{
    if(_status==1) return;
    _socket->connectToHost(_hostname, 14580);

}

void Aprs::disconnectAPRS()
{
    if(_status==0) return;
    _socket->disconnectFromHost();

}

void Aprs::authenticate()
{
    QString _login_text;
    _login_text.append("user ");
    _login_text.append( _aprs_settings );
    _login_text.append(" pass -1 vers 0.8.2 filter r/");
    _login_text.append( QString::number(_init_latitude, 'f', 2) );
    _login_text.append("/");
    _login_text.append(QString::number(_init_longitude, 'f', 2) );
    _login_text.append("/0");
    _login_text.append(QString::number(_plot_range) );
    _login_text.append("\r\n");

    _socket->write(_login_text.toLatin1());
    _socket->flush();
    qDebug() << "Sent APRS login";
    qDebug() << _login_text;

}

void Aprs::processData()
{
    if (_status!=1) return;
    QString response;

    while(_socket->bytesAvailable()>0)
    {
        QByteArray a = _socket->read(1024);
        response.append(a);
    }
    qDebug() << response;

    if(response.contains("verified"))
    {
        _authenticated=1;
        return;
    }
    if(response.startsWith("#"))
    {
        //discard comment
        return;
    }
    else
    {
        //we have a station report
        emit rawAprsData(response);
        QStringList v = response.split(":");
        QStringList v1 = v[0].split(">");
        QString from = v1[0];
        QStringList v2 = v1[1].split(",");
        unsigned via_size = v2.size();
        QString to = v2[0];
        QString via;
        for(uint i = 1;i<via_size ; ++i)
        {
            via.append(v2[i]).append(",");
        }

        QString payload = v[1];


        QString lat;
        QString lon;
        QRegExp re("(\\d\\d\\d\\d\\.\\d\\d\\w)(.)*(\\d\\d\\d\\d\\d.\\d\\d\\w)(\\S)(.+)");
        //QRegularExpressionMatch match = re.match(payload);
        if (re.indexIn(payload)!=-1) {
            lat = re.cap(1);
            lon = re.cap(3);
            QString symbol = re.cap(4);
            QString message = re.cap(5);
            QString lat_degrees;
            QString lon_degrees;
            lat_degrees.append(lat[0]).append(lat[1]);
            lon_degrees.append(lon[0]).append(lon[1]).append(lon[2]);
            QString ns = lat.right(1);
            lat.chop(1);
            QString ew = lon.right(1);
            lon.chop(1);
            lat.remove(0,2);
            lon.remove(0,3);
            double lat_minutes = lat.toDouble();
            double lon_minutes = lon.toDouble();
            double latitude = lat_degrees.toDouble();
            double longitude = lon_degrees.toDouble();
            latitude = latitude + lat_minutes / 60;
            if ( ns.contains("S") ) {
                latitude *= -1;
            }
            longitude = longitude + lon_minutes / 60;
            if ( ew.contains("W") ) {
                longitude *= -1;
            }
            AprsStation *st = new AprsStation;
            st->adressee=to;
            st->callsign = from;
            st->via = via;
            st->symbol = symbol;
            st->payload = payload;
            st->message = message;
            st->latitude = latitude;
            st->longitude = longitude;
            QDateTime dt = QDateTime::currentDateTime();
            st->time_seen = dt.toTime_t();
            emit aprsData(st);

        }



    }

}

void Aprs::setFilter(QPointF &pos, int &range)
{
    if ((_status!=1)) return;
    if( QTime::currentTime() < _delaytime ) return;

    QString query = "#filter r/"+QString::number(pos.ry())+"/"+QString::number(pos.rx())+"/0"+QString::number(range)+"\r\n";

    _socket->write(query.toLatin1());
    _socket->flush();
    _delaytime = QTime::currentTime().addSecs(1);

}

void Aprs::filterCallsign(QString callsign)
{
    if ((_status!=1)) return;
    if( QTime::currentTime() < _delaytime ) return;
    QString query = "#filter b/"+callsign+"\r\n";

    _socket->write(query.toLatin1());
    _socket->flush();
    _delaytime = QTime::currentTime().addSecs(1);
}

void Aprs::filterPrefix(QString prefix)
{
    if ((_status!=1)) return;
    if( QTime::currentTime() < _delaytime ) return;
    QString query = "#filter p/"+prefix+"\r\n";

    _socket->write(query.toLatin1());
    _socket->flush();
    _delaytime = QTime::currentTime().addSecs(1);
}
