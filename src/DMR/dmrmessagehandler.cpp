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

#include "dmrmessagehandler.h"


DMRMessageHandler::DMRMessageHandler(Settings *settings, Logger *logger, QObject *parent) : QObject(parent)
{
    _settings = settings;
    _logger = logger;
}

DMRMessageHandler::~DMRMessageHandler()
{
    QList<unsigned int> keys = _messages.keys();
    for(int i=0;i<keys.size();i++)
    {
        clearMessage(keys[i]);
        clearDataBuffer(keys[i]);
    }
}

void DMRMessageHandler::clearMessage(unsigned int srcId)
{
    if(_messages.contains(srcId))
    {
        data_message *msg = _messages[srcId];
        if(msg != nullptr)
            delete msg;
        _messages.remove(srcId);
    }
}

void DMRMessageHandler::addDataToBuffer(unsigned int srcId, CDMRData &dmr_data)
{
    if(_dmr_data_buffer.contains(srcId))
    {
        QVector<CDMRData>* data_frames = _dmr_data_buffer[srcId];
        data_frames->push_back(dmr_data);
    }
    else
    {
        QVector<CDMRData> *data_frames = new QVector<CDMRData>();
        data_frames->push_back(dmr_data);
        _dmr_data_buffer.insert(dmr_data.getSrcId(), data_frames);
    }
}

QVector<CDMRData>* DMRMessageHandler::getDataFromBuffer(unsigned int srcId)
{
    if(_dmr_data_buffer.contains(srcId))
    {
        QVector<CDMRData> *data_frames = _dmr_data_buffer[srcId];
        return data_frames;
    }
    else
    {
        return nullptr;
    }
}

void DMRMessageHandler::clearDataBuffer(unsigned int srcId)
{
    if(_dmr_data_buffer.contains(srcId))
    {
        QVector<CDMRData> *data_frames = _dmr_data_buffer[srcId];
        data_frames->clear();
        delete data_frames;
        _dmr_data_buffer.remove(srcId);
    }
}

DMRMessageHandler::data_message* DMRMessageHandler::processData(CDMRData &dmr_data)
{
    unsigned int srcId = dmr_data.getSrcId();
    unsigned int dstId = dmr_data.getDstId();
    data_message *msg = nullptr;

    if(dmr_data.getDataType() == DT_DATA_HEADER)
    {
        clearDataBuffer(srcId);
        addDataToBuffer(srcId, dmr_data);
        clearMessage(srcId);
        msg = new data_message;
        _messages.insert(srcId, msg);
        unsigned char data[DMR_FRAME_LENGTH_BYTES];
        dmr_data.getData(data);
        CDMRDataHeader header;
        header.put(data);
        // FIXME: this value should be per channel instead of global
        msg->type = header.getDPF();
        msg->real_dst = dstId;
        msg->real_src = srcId;
        msg->sap = header.getSAP();
        msg->group = header.getGI();
        msg->size = header.getBlocks();
        msg->block = msg->size;
        msg->pad_nibble = header.getPadNibble();
        msg->rssi_accumulator += float(dmr_data.getRSSI()) * -1.0f;
        msg->ber_accumulator += float(dmr_data.getBER()) / 1.41f;
        if(header.getUDT())
        {
            msg->udt_format = header.getUDTFormat();
            msg->udt = true;
            _logger->log(Logger::LogLevelDebug, QString("Received UDT packet data header from %1 to %2 --- A: %3, GI:%4, "
                    "Format: %5, UDT Format: %6, Opcode: %7, RSVD: %8, PF: %9, SF: %10, SAP: %11, No of Blocks: %12,"
                    " Data packet format: %13")
                         .arg(srcId).arg(dstId).arg(header.getA()).arg(header.getGI()).arg(header.getFormat())
                         .arg(header.getUDTFormat()).arg(header.getOpcode()).arg(header.getRSVD()).arg(header.getPF())
                         .arg(header.getSF()).arg(header.getSAP()).arg(header.getBlocks()).arg(header.getDPF()));
        }
        else if(msg->type == DPF_CONFIRMED_DATA)
        {
            if(msg->size > 64)
            {
                clearMessage(srcId);
                return nullptr;
            }
            msg->crc_valid = true;
            msg->udt = false;
            msg->seq_no = header.getSequenceNumber();
            _logger->log(Logger::LogLevelDebug, QString("Received confirmed packet data header from %1 to %2 --- A: %3, GI:%4, "
                    "Format: %5, Pad Nibble: %6, Sequence number: %7, RSVD: %8, PF: %9, SF: %10, SAP: %11, No of Blocks: %12,"
                    " Data packet format: %13")
                         .arg(srcId).arg(dstId).arg(header.getA()).arg(header.getGI()).arg(header.getFormat())
                         .arg(header.getPadNibble()).arg(header.getSequenceNumber()).arg(header.getRSVD()).arg(header.getPF())
                         .arg(header.getSF()).arg(header.getSAP()).arg(header.getBlocks()).arg(header.getDPF()));
        }
    }
    else if((dmr_data.getDataType() == DT_RATE_12_DATA) ||
            (dmr_data.getDataType() == DT_RATE_1_DATA) ||
            (dmr_data.getDataType() == DT_RATE_34_DATA))
    {
        if(_messages.contains(srcId))
        {
            msg = _messages.value(srcId);

            if(msg->size > 0)
            {
                msg->rssi_accumulator += float(dmr_data.getRSSI()) * -1.0f;
                msg->ber_accumulator += float(dmr_data.getBER()) / 1.41f;
                addDataToBuffer(srcId, dmr_data);
                unsigned int block_size = 12U;
                if((dmr_data.getDataType() == DT_RATE_1_DATA))
                    block_size = 24U;
                else if((dmr_data.getDataType() == DT_RATE_34_DATA))
                    block_size = 18U;
                unsigned char block[block_size];
                memset(block, 0, block_size);
                unsigned char data[DMR_FRAME_LENGTH_BYTES];
                dmr_data.getData(data);
                if(dmr_data.getDataType() == DT_RATE_12_DATA)
                {
                    CBPTC19696 bptc;
                    bptc.decode(data, block);
                }
                else if (dmr_data.getDataType() == DT_RATE_34_DATA)
                {
                    CDMRTrellis trellis;
                    bool ret = trellis.decode(data, block);
                    if(!ret)
                    {
                        _logger->log(Logger::LogLevelWarning, QString("Confirmed data block %1 could not be decoded!")
                                     .arg(msg->block));
                    }
                }
                else if (dmr_data.getDataType() == DT_RATE_1_DATA)
                {
                    memcpy(block, data, block_size);
                }
                if((msg->type == DPF_UDT) && (dmr_data.getDataType() == DT_RATE_12_DATA))
                {
                    memcpy(msg->message + ((msg->size - msg->block) * block_size) , block, block_size);
                }
                else if(msg->type == DPF_CONFIRMED_DATA)
                {
                    memcpy(msg->message + ((msg->size - msg->block) * (block_size - 2U)) , block + 2U, block_size - 2U);
                    uint8_t dbsn = 0;
                    bool crc_valid = block_crc(block, block_size, dbsn);
                    if(!crc_valid)
                    {
                        if(msg->sap == 9) // proprietary data
                        {
                            // Last data block seems to contain gibberish, CRC9 check will fail, DBSN also invalid
                            if(msg->block > 1)
                            {
                                msg->crc_valid = false;
                                _logger->log(Logger::LogLevelWarning, QString("Confirmed data block %1 failed CRC9 check")
                                             .arg(dbsn));
                            }
                        }
                        else
                        {
                            msg->crc_valid = false;
                            _logger->log(Logger::LogLevelWarning, QString("Confirmed data block %1 failed CRC9 check")
                                         .arg(dbsn));
                        }
                    }

                }

                // If last block has been received, build the message
                if((dmr_data.getDataType() == DT_RATE_12_DATA) && (msg->block == 1) && (msg->type == DPF_UDT))
                {
                    msg->rssi = msg->rssi_accumulator / float(msg->size + 1);
                    msg->ber = msg->ber_accumulator / float(msg->size + 1);
                    msg->crc_valid = CCRC::checkCCITT162(msg->message, msg->size*block_size);
                    data_message *finished_message = new data_message(msg);
                    clearMessage(srcId);
                    return finished_message;
                }
                else if(msg->block == 1 && msg->type == DPF_CONFIRMED_DATA)
                {
                    if(!msg->crc_valid)
                    {
                        clearMessage(srcId);
                        return nullptr;
                    }
                    bool valid = processConfirmedMessage(msg, block_size);
                    if(!valid)
                    {
                        clearMessage(srcId);
                        return nullptr;
                    }
                    msg->rssi = msg->rssi_accumulator / float(msg->size + 1);
                    msg->ber = msg->ber_accumulator / float(msg->size + 1);
                    if(msg->group)
                    {
                        dstId = Utils::convertBase11GroupNumberToBase10(msg->real_dst);
                    }
                    _logger->log(Logger::LogLevelInfo, QString("Received confirmed data message from %1 to %2 of length %3.")
                                     .arg(msg->real_src).arg(msg->real_dst).arg(msg->payload_len));
                    data_message *finished_message = new data_message(msg);
                    clearMessage(srcId);
                    return finished_message;
                }
                msg->block -= 1;
            }
        }
    }
    _logger->log(Logger::LogLevelDebug, QString("DMR Slot %1, received data packet from %2 to %3")
                 .arg(dmr_data.getSlotNo()).arg(srcId).arg(dstId));

    return nullptr;
}

bool DMRMessageHandler::block_crc(unsigned char *block, unsigned int block_size, uint8_t &dbsn)
{
    dbsn = (block[0] >> 1);
    uint16_t crc_sent = (((block[0] & 1) << 8) | block[1]) ^ 0x0F0;
    unsigned char data[block_size - 1];
    memset(data, 0, block_size - 1);
    memcpy(data, block + 2U, block_size - 2);
    unsigned char s = 0;
    s = (unsigned char)(dbsn << 1);
    memcpy(data + block_size - 2, &s, 1U);
    uint8_t rem = 0;
    unsigned char data_b[11];
    memset(data_b, 0, block_size - 1);
    for(int i=block_size - 2;i>=0;i--)
    {
        if(i > 0)
        {
            rem = data[i-1] & 0x01;
            data_b[i] = data[i] >> 1 | (rem << 7);
        }
        else
        {
            data_b[i] = data[i] >> 1;
        }
    }
    crc_t crc_calc;
    crc_calc = crc9_init();
    crc_calc = crc9_update(crc_calc, data_b, block_size - 1);
    crc_calc = crc9_finalize(crc_calc);
    return crc_calc == crc_sent;
}

bool DMRMessageHandler::message_crc32(data_message *msg, unsigned int type, unsigned int block_size)
{
    unsigned int crc_msg_size;
    unsigned int crc_sent = 0;
    uint index = 0;
    if((msg->sap == 9) && ((type == 0x0201) || (type == 0x0101)))
    {
        // Last block does not contain CRC32 and is not decodable
        crc_msg_size = msg->size*(block_size - 2) - 14 - index;
    }
    else
    {
        crc_msg_size = msg->size*(block_size - 2) - 4;
    }
    crc_sent |= msg->message[index + crc_msg_size + 3];
    crc_sent |= msg->message[index + crc_msg_size + 2] << 8;
    crc_sent |= msg->message[index + crc_msg_size + 1] << 16;
    crc_sent |= msg->message[index + crc_msg_size] << 24;
    unsigned char crc_data[crc_msg_size];
    for(;index <crc_msg_size;index=index+2)
    {
        crc_data[index] = msg->message[index+1];
        crc_data[index+1] = msg->message[index];
    }
    crc_t crc_calculated = crc32_init();
    crc_calculated = crc32_update(crc_calculated, (const unsigned char*)crc_data, crc_msg_size);
    crc_calculated = crc32_finalize(crc_calculated);

    bool valid = crc_calculated == crc_sent;
    if(!valid)
        _logger->log(Logger::LogLevelWarning, QString("Confirmed packet data message failed CRC32 check"));
    return valid;
}

bool DMRMessageHandler::processConfirmedMessage(data_message *msg, unsigned int block_size)
{
    if(!msg->crc_valid)
        return false;
    uint16_t type = 0;
    if(msg->sap == 9)
    {
        type = (msg->message[0] << 8) | msg->message[1];
        //print payload
        for(uint i=0;i<msg->size * (block_size - 2);i++)
        {
            qDebug() << "Byte " << i << " :" << QString::number(msg->message[i], 16) << QString(msg->message[i]);
        }


        if(type == 0x0201)
        {
            if((msg->size*(block_size - 2) - 4) < 40)
                return false;

            msg->payload_len |= msg->message[38];
            msg->payload_len = (msg->payload_len << 8) | msg->message[39];
            if((msg->size*(block_size - 2) - 4) < 41 + msg->payload_len)
                return false;
            memcpy(msg->payload, msg->message + 40, msg->payload_len);
            msg->group = (msg->message[26] & 0x80) == 0x80;
            msg->real_dst = msg->message[27];
            msg->real_dst = (msg->real_dst << 8) | msg->message[28];
            msg->real_dst = (msg->real_dst << 8) | msg->message[29];

            msg->real_src = msg->message[19];
            msg->real_src = (msg->real_src << 8) | msg->message[20];
            msg->real_src = (msg->real_src << 8) | msg->message[21];

            return message_crc32(msg, type, block_size);
        }
        else if(type == 0x0101)
        {

            if((msg->size*(block_size - 2) - 4) < 40)
                return false;

            msg->payload_len |= msg->message[36];
            msg->payload_len = (msg->payload_len << 8) | msg->message[37];
            if((msg->size*(block_size - 2) - 4) < 41 + msg->payload_len)
                return false;
            memcpy(msg->payload, msg->message + 38, msg->payload_len);
            msg->group = (msg->message[24] & 0x80) == 0x80;
            msg->real_dst = msg->message[25];
            msg->real_dst = (msg->real_dst << 8) | msg->message[26];
            msg->real_dst = (msg->real_dst << 8) | msg->message[27];

            msg->real_src = msg->message[17];
            msg->real_src = (msg->real_src << 8) | msg->message[18];
            msg->real_src = (msg->real_src << 8) | msg->message[19];

            return message_crc32(msg, type, block_size);
        }
    }
    else
    {
        msg->payload_len = msg->size*(block_size - 2) - 4;
        memcpy(msg->payload, msg->message, msg->payload_len);
        return message_crc32(msg, type, block_size);
    }
    return false;
}
