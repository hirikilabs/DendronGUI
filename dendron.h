#ifndef DENDRON_H
#define DENDRON_H

#include <inttypes.h>

#define DENDRON_PACKET_SIZE         48
#define DENDRON_CHANNEL_MODE_TEST   't'
#define DENDRON_CHANNEL_MODE_GROUND 'g'
#define DENDRON_CHANNEL_MODE_NORMAL 'n'

#define DENDRON_GET_BATTERY 'b'

typedef int32_t channel_data_t[8];

typedef struct {
    uint32_t packet_num;
    uint32_t status;
    channel_data_t data;
    float battery;
} dendron_data_t;


#endif // DENDRON_H
