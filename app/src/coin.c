#include "coin.h"
#include <stdint.h>
#include <stddef.h>

#define MAX_CHAIN_ID_LEN 20

static const char SUPPORTED_CHAIN_IDS[SUPPORTED_CHAIN_COUNT][MAX_CHAIN_ID_LEN] = {
    "thorchain",
    "thorchain-1",
    "thorchain-stagenet-2"
};

const char* get_supported_chain(uint8_t index) {
    if (index >= SUPPORTED_CHAIN_COUNT) return NULL;
    return SUPPORTED_CHAIN_IDS[index];
}
