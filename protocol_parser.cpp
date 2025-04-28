#include "protocol_parser.h"

struct protocol_parser parser;

static uint16_t update_crc(uint16_t crc, uint8_t byte)
{
    return crc16_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
}

static uint16_t calculate_crc(const uint8_t* array, int size) {
   uint16_t crc = CRC_INIT; // #define CRC_INIT 0xffff
   int i;
   for (i = 0; i < size; i++) {
       crc = update_crc(crc, array[i]);
   }
   return crc;
}

void serialize_reply(transfer* data) {
    if (data == nullptr) return;

    uint16_t crc;
    uint16_t PAYLOAD_SIZE;

        if (data->getCmd() == 0) {
            PAYLOAD_SIZE = 1;
            data->buf[0] = SYNC_BYTE;
            data->buf[1] = ((PAYLOAD_SIZE + DATA_SIZE_OFFSET) >> 0) & 0xff;
            data->buf[2] = ((PAYLOAD_SIZE + DATA_SIZE_OFFSET) >> 8) & 0xff;
            data->buf[3] = data->getCmd();
            data->buf[4] = data->getStatus();
            crc = calculate_crc(data->buf + 3, PAYLOAD_SIZE + 1);
            data->buf[5] = (crc >> 0) & 0xff;
            data->buf[6] = (crc >> 8) & 0xff;
            return;
        } else {
            PAYLOAD_SIZE = 5;
            data->buf[0] = SYNC_BYTE;
            data->buf[1] = ((PAYLOAD_SIZE + DATA_SIZE_OFFSET) >> 0) & 0xff;
            data->buf[2] = ((PAYLOAD_SIZE + DATA_SIZE_OFFSET) >> 8) & 0xff;
            data->buf[3] = data->getCmd();
            data->buf[4] = data->getStatus();
            uint8_t* value = data->getValue();
            for (size_t i = 5; i < 9; i++)
            {
                data->buf[i] = value[i - 5];
            }
            crc = calculate_crc(data->buf + 3, PAYLOAD_SIZE + 1);
            data->buf[9] = (crc >> 0) & 0xff;
            data->buf[10] = (crc >> 8) & 0xff;
            }

}

enum parser_result process_rx_byte(struct protocol_parser *parser, uint8_t byte) {
    enum parser_result ret = PARSER_OK;

    switch (parser->state) {
    case protocol_parser::STATE_SYNC:
        if (byte == SYNC_BYTE) {
            parser->state = protocol_parser::STATE_SIZE_L;
            parser->crc = CRC_INIT;
            parser->buffer_length = 0;
        }
        break;
    case protocol_parser::STATE_SIZE_L:
        parser->data_size = byte;
        parser->state = protocol_parser::STATE_SIZE_H;
        break;
    case protocol_parser::STATE_SIZE_H:
        parser->data_size |= ((uint16_t)byte << 8);
        if (parser->data_size >= DATA_SIZE_OFFSET &&
                parser->data_size <= MAX_DATA_SIZE + DATA_SIZE_OFFSET) {
            parser->state = protocol_parser::STATE_CMD;
        } else {
            parser->state = protocol_parser::STATE_SYNC;
            ret = PARSER_ERROR;
        }
        break;
    case protocol_parser::STATE_CMD:
        parser->crc = update_crc(parser->crc, byte);
        parser->cmd = byte;
        parser->state = (parser->data_size != DATA_SIZE_OFFSET) ? protocol_parser::STATE_DATA : protocol_parser::STATE_CRC_L;
        break;
    case protocol_parser::STATE_DATA:
        parser->crc = update_crc(parser->crc, byte);
        parser->buffer[parser->buffer_length++] = byte;
        if (parser->buffer_length + DATA_SIZE_OFFSET >= parser->data_size) {
            parser->state = protocol_parser::STATE_CRC_L;
        }
        break;
    case protocol_parser::STATE_CRC_L:
        parser->crc = update_crc(parser->crc, byte);
        parser->state = protocol_parser::STATE_CRC_H;
        break;
    case protocol_parser::STATE_CRC_H:
        parser->crc = update_crc(parser->crc, byte);
        parser->state = protocol_parser::STATE_SYNC;
        ret = (parser->crc == 0 ? PARSER_DONE : PARSER_ERROR);
        break;
    default:
        ret = PARSER_ERROR;
    }

    return ret;
}


