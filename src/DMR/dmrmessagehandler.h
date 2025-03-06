// Written by Adrian Musceac YO8RZZ , started September 2024.
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

#ifndef DMRMESSAGEHANDLER_H
#define DMRMESSAGEHANDLER_H

#include <QObject>
#include <QMap>
#include <QVector>
#include "settings.h"
#include "logger.h"
#include "crc9.h"
#include "crc32.h"
#include "utils.h"
#include "MMDVM/DMRData.h"
#include "MMDVM/DMRDataHeader.h"
#include "MMDVM/BPTC19696.h"
#include "MMDVM/DMRTrellis.h"
#include "MMDVM/CRC.h"

class DMRMessageHandler : public QObject
{
    Q_OBJECT
public:
    struct data_message
    {
        data_message() : size(0), type(0), pad_nibble(0), block(0), udt_format(0), sap(0),
            payload_len(0), real_src(0), real_dst(0), seq_no(0), group(false), udt(true), crc_valid(false),
            rssi_accumulator(0.0f), ber_accumulator(0.0f), rssi(0.0f), ber(0.0f) {
            memset(message, 0, 1024);
            memset(payload, 0, 1024);
        }
        data_message(data_message *msg) : size(msg->size), type(msg->type), pad_nibble(msg->pad_nibble), block(msg->block),
            udt_format(msg->udt_format), sap(msg->sap), payload_len(msg->payload_len),
            real_src(msg->real_src), real_dst(msg->real_dst), seq_no(msg->seq_no), group(msg->group), udt(msg->udt),
            crc_valid(msg->crc_valid), rssi_accumulator(msg->rssi_accumulator), ber_accumulator(msg->ber_accumulator),
            rssi(msg->rssi), ber(msg->ber) {
            memcpy(message, msg->message, 1024);
            memcpy(payload, msg->payload, 1024);
        }
        unsigned int size;
        unsigned int type;
        unsigned int pad_nibble;
        unsigned int block;
        unsigned int udt_format;
        unsigned int sap;
        unsigned char message[1024];
        unsigned char payload[1024];
        unsigned int payload_len;
        unsigned int real_src;
        unsigned int real_dst;
        unsigned int seq_no;
        bool group;
        bool udt;
        bool crc_valid;
        float rssi_accumulator;
        float ber_accumulator;
        float rssi;
        float ber;
    };
    explicit DMRMessageHandler(Settings *settings, Logger *logger, QObject *parent = nullptr);
    ~DMRMessageHandler();
    data_message *processData(CDMRData &dmr_data);
    void addDataToBuffer(unsigned int srcId, CDMRData &dmr_data);
    QVector<CDMRData> *getDataFromBuffer(unsigned int srcId);
    void clearDataBuffer(unsigned int srcId);


signals:


private:
    Settings *_settings;
    Logger *_logger;
    bool block_crc(unsigned char *block, unsigned int block_size, uint8_t &dbsn);
    bool message_crc32(data_message *msg, unsigned int type, unsigned int block_size);
    bool processConfirmedMessage(data_message *msg, unsigned int block_size);
    void clearMessage(unsigned int srcId);

    QMap<unsigned int, data_message*> _messages;
    QMap<unsigned int, QVector<CDMRData>*> _dmr_data_buffer;

};

#endif // DMRMESSAGEHANDLER_H
