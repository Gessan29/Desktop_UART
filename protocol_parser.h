#ifndef PROTOCOL_PARSER_H
#define PROTOCOL_PARSER_H
#include <stdint.h>
#include <stddef.h>

#define MAX_DATA_SIZE 201 // Максимальный размер буфера.
#define SYNC_BYTE 0xAA // Синхробайт.
#define DATA_SIZE_OFFSET 3 // 2 байта crc + код команды.
#define SIZE_PAKET 7 // синхробайт + 2 байта полезных данных + cmd + status + 2 CRC.
#define CRC_INIT 0xffff // для подсчета контрольной суммы CRC.

enum parser_result {
    PARSER_OK,
    PARSER_ERROR,
    PARSER_DONE,
};

struct protocol_parser {
    enum {
        STATE_SYNC,
        STATE_SIZE_L,
        STATE_SIZE_H,
        STATE_CMD,
        STATE_DATA,
        STATE_CRC_L,
        STATE_CRC_H,
    } state;

    uint8_t buffer[MAX_DATA_SIZE];  // Буфер для хранения данных пакета.
    size_t buffer_length;         // Количество принятых байт данных.
    uint16_t data_size;             // Размер полезных данных, полученный из пакета с учетом DATA_SIZE_OFFSET.
    uint8_t cmd;                    // Команда пакета.
    uint16_t crc;                   // Накопленная контрольная сумма.
};

struct for_transfer
{
    uint8_t* buf;
    size_t buf_size;
    uint8_t cmd;
    uint8_t status;
    uint8_t* value;
};

//static uint16_t update_crc(uint16_t crc, uint8_t byte);
enum parser_result process_rx_byte(struct protocol_parser *parser, uint8_t byte);
void serialize_reply(struct for_transfer* data);
#endif // PROTOCOL_PARSER_H
