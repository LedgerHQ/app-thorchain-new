/*******************************************************************************
*   (c) 2023 Zondax AG
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

#include "crypto.h"
#include "coin.h"
#include "zxmacros.h"
#include "apdu_codes.h"
#include "tx.h"

#include <bech32.h>
#include "chain_config.h"

#define MAX_DER_SIGNATURE_LEN   73u

uint32_t hdPath[HDPATH_LEN_DEFAULT];

uint8_t bech32_hrp_len;
char bech32_hrp[MAX_BECH32_HRP_LEN + 1];
address_encoding_e encoding = BECH32_COSMOS;

#include "cx.h"

static zxerr_t crypto_extractUncompressedPublicKey(uint8_t *pubKey, uint16_t pubKeyLen) {
    if (pubKey == NULL || pubKeyLen < PK_LEN_SECP256K1_UNCOMPRESSED) {
        return zxerr_invalid_crypto_settings;
    }

    cx_ecfp_public_key_t cx_publicKey = {0};
    cx_ecfp_private_key_t cx_privateKey = {0};
    uint8_t privateKeyData[64] = {0};

    zxerr_t error = zxerr_unknown;
    // Generate keys
    CATCH_CXERROR(os_derive_bip32_with_seed_no_throw(HDW_NORMAL,
                                                     CX_CURVE_256K1,
                                                     hdPath,
                                                     HDPATH_LEN_DEFAULT,
                                                     privateKeyData,
                                                     NULL,
                                                     NULL,
                                                     0));

    CATCH_CXERROR(cx_ecfp_init_private_key_no_throw(CX_CURVE_256K1, privateKeyData, 32, &cx_privateKey));
    CATCH_CXERROR(cx_ecfp_init_public_key_no_throw(CX_CURVE_256K1, NULL, 0, &cx_publicKey));
    CATCH_CXERROR(cx_ecfp_generate_pair_no_throw(CX_CURVE_256K1, &cx_publicKey, &cx_privateKey, 1));
    memcpy(pubKey, cx_publicKey.W, PK_LEN_SECP256K1_UNCOMPRESSED);
    error = zxerr_ok;

catch_cx_error:
    MEMZERO(&cx_privateKey, sizeof(cx_privateKey));
    MEMZERO(privateKeyData, sizeof(privateKeyData));

    if (error != zxerr_ok) {
        MEMZERO(pubKey, pubKeyLen);
    }

    return error;
}

__Z_INLINE zxerr_t compressPubkey(const uint8_t *pubkey, uint16_t pubkeyLen, uint8_t *output, uint16_t outputLen) {
    if (pubkey == NULL || output == NULL ||
        pubkeyLen != PK_LEN_SECP256K1_UNCOMPRESSED || outputLen < PK_LEN_SECP256K1) {
            return zxerr_invalid_crypto_settings;
    }

    MEMCPY(output, pubkey, PK_LEN_SECP256K1);
    output[0] = pubkey[64] & 1 ? 0x03 : 0x02; // "Compress" public key in place
    return zxerr_ok;
}

static zxerr_t crypto_hashBuffer(const uint8_t *input, const uint16_t inputLen,
                          uint8_t *output, uint16_t outputLen) {

    switch (encoding) {
        case BECH32_COSMOS: {
            cx_hash_sha256(input, inputLen, output, outputLen);
            break;
        }

        case BECH32_ETH: {
            cx_sha3_t sha3 = {0};
            cx_err_t status = cx_keccak_init_no_throw(&sha3, 256);
            if (status != CX_OK) {
                 return zxerr_ledger_api_error;
            }
            status = cx_hash_no_throw((cx_hash_t*) &sha3, CX_LAST, input, inputLen, output, outputLen);
            if (status != CX_OK) {
                return zxerr_ledger_api_error;
            }
            break;
        }

        default:
            return zxerr_unknown;
    }
    return zxerr_ok;
}

zxerr_t crypto_sign(uint8_t *output,
                   uint16_t outputLen,
                   uint16_t *sigSize) {
    if (output == NULL || sigSize == NULL || outputLen < MAX_DER_SIGNATURE_LEN) {
            return zxerr_invalid_crypto_settings;
    }

    uint8_t messageDigest[CX_SHA256_SIZE] = {0};

    // Hash it
    const uint8_t *message = tx_get_buffer();
    const uint16_t messageLen = tx_get_buffer_length();

    CHECK_ZXERR(crypto_hashBuffer(message, messageLen, messageDigest, CX_SHA256_SIZE));
    CHECK_APP_CANARY()

    cx_ecfp_private_key_t cx_privateKey;
    uint8_t privateKeyData[64] = {0};
    size_t signatureLength = MAX_DER_SIGNATURE_LEN;
    uint32_t tmpInfo = 0;
    *sigSize = 0;

    zxerr_t error = zxerr_unknown;

    CATCH_CXERROR(os_derive_bip32_with_seed_no_throw(HDW_NORMAL,
                                                     CX_CURVE_256K1,
                                                     hdPath,
                                                     HDPATH_LEN_DEFAULT,
                                                     privateKeyData,
                                                     NULL,
                                                     NULL,
                                                     0));
    CATCH_CXERROR(cx_ecfp_init_private_key_no_throw(CX_CURVE_256K1, privateKeyData, 32, &cx_privateKey));
    CATCH_CXERROR(cx_ecdsa_sign_no_throw(&cx_privateKey,
                                         CX_RND_RFC6979 | CX_LAST,
                                         CX_SHA256,
                                         messageDigest,
                                         CX_SHA256_SIZE,
                                         output,
                                         &signatureLength, &tmpInfo));
    *sigSize = signatureLength;
    error = zxerr_ok;

catch_cx_error:
    MEMZERO(&cx_privateKey, sizeof(cx_privateKey));
    MEMZERO(privateKeyData, sizeof(privateKeyData));

    if (error != zxerr_ok) {
        MEMZERO(output, outputLen);
    }

    return error;
}

static zxerr_t ripemd160_32(uint8_t *out, uint8_t *in) {
    cx_ripemd160_t rip160;
    CHECK_CX_OK(cx_ripemd160_init_no_throw(&rip160));
    CHECK_CX_OK(cx_hash_no_throw(&rip160.header, CX_LAST, in, CX_SHA256_SIZE, out, CX_RIPEMD160_SIZE));
    return zxerr_ok;
}

zxerr_t crypto_fillAddress(uint8_t *buffer, uint16_t buffer_len, uint16_t *addrResponseLen) {
    if (buffer_len < PK_LEN_SECP256K1 + 50) {
        return zxerr_buffer_too_small;
    }

    // extract pubkey
    uint8_t uncompressedPubkey [PK_LEN_SECP256K1_UNCOMPRESSED] = {0};
    CHECK_ZXERR(crypto_extractUncompressedPublicKey(uncompressedPubkey, sizeof(uncompressedPubkey)));
    CHECK_ZXERR(compressPubkey(uncompressedPubkey, sizeof(uncompressedPubkey), buffer, buffer_len));
    char *addr = (char *) (buffer + PK_LEN_SECP256K1);

    uint8_t hashed1_pk[CX_SHA256_SIZE] = {0};

    switch (encoding) {
        case BECH32_COSMOS: {
            // Hash it
            cx_hash_sha256(buffer, PK_LEN_SECP256K1, hashed1_pk, CX_SHA256_SIZE);
            uint8_t hashed2_pk[CX_RIPEMD160_SIZE] = {0};
            CHECK_ZXERR(ripemd160_32(hashed2_pk, hashed1_pk));
            CHECK_ZXERR(bech32EncodeFromBytes(addr, buffer_len - PK_LEN_SECP256K1, bech32_hrp, hashed2_pk, CX_RIPEMD160_SIZE, 1, BECH32_ENCODING_BECH32));
            break;
        }

        case BECH32_ETH: {
            cx_sha3_t ctx;
            if (cx_keccak_init_no_throw(&ctx, 256) != CX_OK) {
                return zxerr_unknown;
            }
            CHECK_CX_OK(cx_hash_no_throw((cx_hash_t *)&ctx, CX_LAST, uncompressedPubkey+1, sizeof(uncompressedPubkey)-1, hashed1_pk, sizeof(hashed1_pk)));
            CHECK_ZXERR(bech32EncodeFromBytes(addr, buffer_len - PK_LEN_SECP256K1, bech32_hrp, hashed1_pk + ETH_ADDRESS_OFFSET, sizeof(hashed1_pk) - ETH_ADDRESS_OFFSET, 1, BECH32_ENCODING_BECH32));
            break;
        }

        default:
            *addrResponseLen = 0;
            return zxerr_encoding_failed;
    }

    *addrResponseLen = PK_LEN_SECP256K1 + strnlen(addr, (buffer_len - PK_LEN_SECP256K1));

    return zxerr_ok;
}
