#ifndef PCBWRITER_DEBUGDATA_H
#define PCBWRITER_DEBUGDATA_H

#include <inttypes.h>

struct debug_data_t {
    uint32_t seq_num;
    uint16_t speed;
    uint16_t des_speed;
    uint16_t control;
    uint32_t delta;
} __attribute__((packed));

#endif
