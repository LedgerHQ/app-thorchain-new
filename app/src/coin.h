/*******************************************************************************
*  (c) 2019-2021 Zondax GmbH
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#define CLA                  0x55u

#define HDPATH_LEN_DEFAULT   5

#define HDPATH_0_DEFAULT     (0x80000000u | 0x2cu)
#define HDPATH_1_DEFAULT     (0x80000000u | 0x3a3u)
#define HDPATH_2_DEFAULT     (0x80000000u | 0u)
#define HDPATH_3_DEFAULT     (0u)

#define PK_LEN_SECP256K1     33u
#define PK_LEN_SECP256K1_UNCOMPRESSED   65u

typedef enum {
    addr_secp256k1 = 0,
} address_kind_e;

typedef enum {
    tx_json = 0,
} tx_type_e;

typedef enum {
    BECH32_COSMOS = 0,
    BECH32_ETH,
    UNSUPPORTED = 0xFF,
} address_encoding_e;

#define VIEW_ADDRESS_OFFSET_SECP256K1       PK_LEN_SECP256K1
#define VIEW_ADDRESS_LAST_PAGE_DEFAULT      0

#define MENU_MAIN_APP_LINE1                "THORChain"
#define MENU_MAIN_APP_LINE2                "Ready"
#define APPVERSION_LINE1                   "Version:"
#define APPVERSION_LINE2                   ("v" APPVERSION)

#define COIN_DEFAULT_DENOM_BASE             "rune"
#define COIN_DEFAULT_DENOM_REPR             "RUNE"
#define COIN_DEFAULT_DENOM_REPR_2           "THOR.RUNE"
#define COIN_DEFAULT_DENOM_FACTOR           8u
#define COIN_DEFAULT_DENOM_TRIMMING         0u

// Coin denoms may be up to 128 characters long
// https://github.com/cosmos/cosmos-sdk/blob/master/types/coin.go#L780
// https://github.com/cosmos/ibc-go/blob/main/docs/architecture/adr-001-coin-source-tracing.md
#define COIN_DENOM_MAXSIZE                  129
#define COIN_AMOUNT_MAXSIZE                 50

#define COIN_MAX_CHAINID_LEN                20u
#define INDEXING_TMP_KEYSIZE                70u
#define INDEXING_TMP_VALUESIZE              70u
#define INDEXING_GROUPING_REF_TYPE_SIZE     70u
#define INDEXING_GROUPING_REF_FROM_SIZE     70u

#define MENU_MAIN_APP_LINE2_SECRET         "?"
#define COIN_SECRET_REQUIRED_CLICKS         0

#define INS_GET_VERSION                 0x00
#define INS_SIGN_SECP256K1              0x02u
#define INS_GET_ADDR_SECP256K1          0x04u

#define SUPPORTED_CHAIN_COUNT 3

const char* get_supported_chain(uint8_t index);

#ifdef __cplusplus
}
#endif
