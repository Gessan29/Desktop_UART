#include "protocol_parser.h"

struct protocol_parser parser;
static const uint16_t crc16_table[256];

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

void serialize_reply(struct for_transfer* data) {
    uint16_t crc;
    static uint16_t PAYLOAD_SIZE;

        if (data->cmd == 0)
        {
            data->buf = (uint8_t*)malloc(7 * sizeof(uint8_t));
            if (data->buf == NULL)
            { return; }

            PAYLOAD_SIZE = 1;
            data->buf[0] = SYNC_BYTE;
            data->buf[1] = ((PAYLOAD_SIZE + DATA_SIZE_OFFSET) >> 0) & 0xff;
            data->buf[2] = ((PAYLOAD_SIZE + DATA_SIZE_OFFSET) >> 8) & 0xff;
            data->buf[3] = data->cmd;
            data->buf[4] = data->status;
            crc = calculate_crc(data->buf + 3, PAYLOAD_SIZE + 1);
            data->buf[5] = (crc >> 0) & 0xff;
            data->buf[6] = (crc >> 8) & 0xff;
            return;
        }
        data->buf = (uint8_t*)malloc(11 * sizeof(uint8_t));
        if (data->buf == NULL)
        { return; }
        PAYLOAD_SIZE = 5;
        data->buf[0] = SYNC_BYTE;
        data->buf[1] = ((PAYLOAD_SIZE + DATA_SIZE_OFFSET) >> 0) & 0xff;
        data->buf[2] = ((PAYLOAD_SIZE + DATA_SIZE_OFFSET) >> 8) & 0xff;
        data->buf[3] = data->cmd;
        data->buf[4] = data->status;
        int a = 0;
        for (size_t i = 5; i < 9; i++)
        {
            data->buf[i] = data->value[a];
            a++;
        }
        crc = calculate_crc(data->buf + 3, PAYLOAD_SIZE + 1);
        data->buf[9] = (crc >> 0) & 0xff;
        data->buf[10] = (crc >> 8) & 0xff;

}

enum parser_result process_rx_byte(struct protocol_parser *parser, uint8_t byte) {
    enum parser_result ret = PARSER_OK;

    switch (parser->state) {
    case STATE_SYNC:
        if (byte == SYNC_BYTE) {
            parser->state = STATE_SIZE_L;
            parser->crc = CRC_INIT;
            parser->buffer_length = 0;
        }
        break;
    case STATE_SIZE_L:
        parser->data_size = byte;
        parser->state = STATE_SIZE_H;
        break;
    case STATE_SIZE_H:
        parser->data_size |= ((uint16_t)byte << 8);
        if (parser->data_size >= DATA_SIZE_OFFSET &&
                parser->data_size <= MAX_DATA_SIZE + DATA_SIZE_OFFSET) {
            parser->state = STATE_CMD;
        } else {
            parser->state = STATE_SYNC;
            ret = PARSER_ERROR;
        }
        break;
    case STATE_CMD:
        parser->crc = update_crc(parser->crc, byte);
        parser->cmd = byte;
        parser->state = (parser->data_size != DATA_SIZE_OFFSET) ? STATE_DATA : STATE_CRC_L;
        break;
    case STATE_DATA:
        parser->crc = update_crc(parser->crc, byte);
        parser->buffer[parser->buffer_length++] = byte;
        if (parser->buffer_length + DATA_SIZE_OFFSET >= parser->data_size) {
            parser->state = STATE_CRC_L;
        }
        break;
    case STATE_CRC_L:
        parser->crc = update_crc(parser->crc, byte);
        parser->state = STATE_CRC_H;
        break;
    case STATE_CRC_H:
        parser->crc = update_crc(parser->crc, byte);
        parser->state = STATE_SYNC;
        ret = (parser->crc == 0 ? PARSER_DONE : PARSER_ERROR);
        break;
    }

    return ret;
}

static const uint16_t crc16_table[256] =
{
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
};
