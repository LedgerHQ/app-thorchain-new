/*******************************************************************************
*   (c) 2019 Zondax GmbH
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

#include <stdio.h>
#include <zxmacros.h>
#include <zxformat.h>
#include <tx_validate.h>
#include <zxtypes.h>
#include "tx_parser.h"
#include "tx_display.h"
#include "parser_impl.h"
#include "common/parser.h"
#include "coin.h"
#include "app_mode.h"
#include <cbor/cbor_parser_helper.h>

parser_error_t parser_init_context(parser_context_t *ctx,
                                   const uint8_t *buffer,
                                   uint16_t bufferSize) {
    ctx->offset = 0;

    if (bufferSize == 0 || buffer == NULL) {
        // Not available, use defaults
        ctx->buffer = NULL;
        ctx->bufferLen = 0;
        return parser_init_context_empty;
    }

    ctx->buffer = buffer;
    ctx->bufferLen = bufferSize;

    return parser_ok;
}

parser_error_t parser_parse(parser_context_t *ctx,
                            const uint8_t *data,
                            size_t dataLen,
                            parser_tx_t *tx_obj) {

    CHECK_PARSER_ERR(parser_init_context(ctx, data, dataLen))
    ctx->tx_obj = tx_obj;
    CHECK_PARSER_ERR(_read_json_tx(ctx, tx_obj))

    extraDepthLevel = false;
    return parser_ok;
}

parser_error_t parser_validate(const parser_context_t *ctx) {
    if (ctx->tx_obj->tx_type == tx_json) {
          CHECK_PARSER_ERR(tx_validate(&parser_tx_obj.tx_json.json))
    }

    // Iterate through all items to check that all can be shown and are valid
    uint8_t numItems = 0;
    CHECK_PARSER_ERR(parser_getNumItems(&numItems))

    char tmpKey[40];
    char tmpVal[40];
    uint8_t pageCount = 0;
    for (uint8_t idx = 0; idx < numItems; idx++) {
        CHECK_PARSER_ERR(parser_getItem(idx, tmpKey, sizeof(tmpKey), tmpVal, sizeof(tmpVal), 0, &pageCount))
    }
    return parser_ok;
}

parser_error_t parser_getNumItems(uint8_t *num_items) {
    *num_items = 0;

    return tx_display_numItems(num_items);
}

__Z_INLINE bool parser_isAmount(char *key) {
    if (strcmp(key, "msgs/value/coins") == 0){
        return true;
    }

    if (strcmp(key, "fee/amount") == 0) {
        return true;
    }

    if (strcmp(key, "msgs/value/amount") == 0) {
        return true;
    }

    return false;
}

__Z_INLINE parser_error_t is_default_denom_base(const char *denom, uint8_t denom_len, bool *is_default) {
    if (is_default == NULL) {
        return parser_unexpected_value;
    }

    bool is_expert_or_default = false;
    CHECK_PARSER_ERR(tx_is_expert_mode_or_not_default_chainid(&is_expert_or_default))
    if (is_expert_or_default) {
        *is_default = false;
        return parser_ok;
    }

    if (strlen(COIN_DEFAULT_DENOM_BASE) != denom_len && strlen(COIN_DEFAULT_DENOM_REPR_2) != denom_len) {
        *is_default = false;
        return parser_ok;
    }

    if (memcmp(denom, COIN_DEFAULT_DENOM_BASE, denom_len) == 0 || memcmp(denom, COIN_DEFAULT_DENOM_REPR_2, denom_len) == 0) {
        *is_default = true;
        return parser_ok;
    }

    return parser_ok;
}

__Z_INLINE parser_error_t parser_formatAmountItem(uint16_t amountToken,
                                                  char *outVal, uint16_t outValLen,
                                                  uint8_t pageIdx, uint8_t *pageCount, bool *is_default) {
    *pageCount = 0;

    uint16_t numElements;
    CHECK_PARSER_ERR(array_get_element_count(&parser_tx_obj.tx_json.json, amountToken, &numElements))

    if (numElements == 0) {
        *pageCount = 1;
        snprintf(outVal, outValLen, "Empty");
        return parser_ok;
    }

    if (numElements != 4) {
        return parser_unexpected_field;
    }

    if (parser_tx_obj.tx_json.json.tokens[amountToken].type != JSMN_OBJECT) {
        return parser_unexpected_field;
    }

    if (parser_tx_obj.tx_json.json.tokens[amountToken + 2].type != JSMN_STRING) {
        return parser_unexpected_field;
    }

    char bufferUI[160];
    char tmpDenom[COIN_DENOM_MAXSIZE];
    char tmpAmount[COIN_AMOUNT_MAXSIZE];
    MEMZERO(tmpDenom, sizeof tmpDenom);
    MEMZERO(tmpAmount, sizeof(tmpAmount));
    MEMZERO(outVal, outValLen);
    MEMZERO(bufferUI, sizeof(bufferUI));

    if (parser_tx_obj.tx_json.json.tokens[amountToken + 2].start < 0 ||
        parser_tx_obj.tx_json.json.tokens[amountToken + 4].start < 0) {
        return parser_unexpected_buffer_end;
    }
    const char *amountPtr = parser_tx_obj.tx_json.tx + parser_tx_obj.tx_json.json.tokens[amountToken + 2].start;

    const int32_t amountLen = parser_tx_obj.tx_json.json.tokens[amountToken + 2].end -
                              parser_tx_obj.tx_json.json.tokens[amountToken + 2].start;
    const char *denomPtr = parser_tx_obj.tx_json.tx + parser_tx_obj.tx_json.json.tokens[amountToken + 4].start;
    const int32_t denomLen = parser_tx_obj.tx_json.json.tokens[amountToken + 4].end -
                             parser_tx_obj.tx_json.json.tokens[amountToken + 4].start;

    if (denomLen <= 0 || denomLen >= COIN_DENOM_MAXSIZE) {
        return parser_unexpected_error;
    }
    if (amountLen <= 0 || amountLen >= COIN_AMOUNT_MAXSIZE) {
        return parser_unexpected_error;
    }

    const size_t totalLen = amountLen + denomLen + 2;
    if (sizeof(bufferUI) < totalLen) {
        return parser_unexpected_buffer_end;
    }

    // Extract amount and denomination
    MEMCPY(tmpDenom, denomPtr, denomLen);
    MEMCPY(tmpAmount, amountPtr, amountLen);

    snprintf(bufferUI, sizeof(bufferUI), "%s ", tmpAmount);
    // If denomination has been recognized format and replace
    *is_default = false;
    CHECK_PARSER_ERR(is_default_denom_base(denomPtr, denomLen, is_default))
    if (*is_default) {
        if (fpstr_to_str(bufferUI, sizeof(bufferUI), tmpAmount, COIN_DEFAULT_DENOM_FACTOR) != 0) {
            return parser_unexpected_error;
        }
        number_inplace_trimming(bufferUI, COIN_DEFAULT_DENOM_TRIMMING);
        snprintf(tmpDenom, sizeof(tmpDenom), " %s", COIN_DEFAULT_DENOM_REPR);
    }

    z_str3join(bufferUI, sizeof(bufferUI), "", tmpDenom);
    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);

    return parser_ok;
}

__Z_INLINE parser_error_t parser_formatAmount(uint16_t amountToken,
                                              char *outVal, uint16_t outValLen,
                                              uint8_t pageIdx, uint8_t *pageCount, bool *is_default) {
    ZEMU_LOGF(200, "[formatAmount] ------- pageidx %d\n", pageIdx)

    *pageCount = 0;
    if (parser_tx_obj.tx_json.json.tokens[amountToken].type != JSMN_ARRAY) {
        return parser_formatAmountItem(amountToken, outVal, outValLen, pageIdx, pageCount, is_default);
    }

    uint8_t totalPages = 0;
    uint8_t showItemSet = 0;
    uint8_t showPageIdx = pageIdx;
    uint16_t showItemTokenIdx = 0;

    uint16_t numberAmounts;
    CHECK_PARSER_ERR(array_get_element_count(&parser_tx_obj.tx_json.json, amountToken, &numberAmounts))

    // Count total subpagesCount and calculate correct page and TokenIdx
    for (uint16_t i = 0; i < numberAmounts; i++) {
        uint16_t itemTokenIdx;
        uint8_t subpagesCount;

        CHECK_PARSER_ERR(array_get_nth_element(&parser_tx_obj.tx_json.json, amountToken, i, &itemTokenIdx));
        CHECK_PARSER_ERR(parser_formatAmountItem(itemTokenIdx, outVal, outValLen, 0, &subpagesCount, is_default));
        totalPages += subpagesCount;

        ZEMU_LOGF(200, "[formatAmount] [%d] TokenIdx: %d - PageIdx: %d - Pages: %d - Total %d", i, itemTokenIdx,
                  showPageIdx, subpagesCount, totalPages)

        if (!showItemSet) {
            if (showPageIdx < subpagesCount) {
                showItemSet = 1;
                showItemTokenIdx = itemTokenIdx;
                ZEMU_LOGF(200, "[formatAmount] [%d] [SET] TokenIdx %d - PageIdx: %d", i, showItemTokenIdx,
                          showPageIdx)
            } else {
                showPageIdx -= subpagesCount;
            }
        }
    }
    *pageCount = totalPages;
    if (pageIdx > totalPages) {
        return parser_unexpected_value;
    }

    if (totalPages == 0) {
        *pageCount = 1;
        snprintf(outVal, outValLen, "Empty");
        return parser_ok;
    }

    uint8_t dummy;
    return parser_formatAmountItem(showItemTokenIdx, outVal, outValLen, showPageIdx, &dummy, is_default);
}

__Z_INLINE parser_error_t parser_getJsonItem(uint8_t displayIdx,
                              char *outKey, uint16_t outKeyLen,
                              char *outVal, uint16_t outValLen,
                              uint8_t pageIdx, uint8_t *pageCount) {

    *pageCount = 0;
    char tmpKey[35] = {0};

    MEMZERO(outKey, outKeyLen);
    MEMZERO(outVal, outValLen);

    uint8_t numItems;
    CHECK_PARSER_ERR(parser_getNumItems(&numItems))
    CHECK_APP_CANARY()

    if (numItems == 0) {
        return parser_unexpected_number_items;
    }

    if (displayIdx >= numItems) {
        return parser_display_idx_out_of_range;
    }

    uint16_t ret_value_token_index = 0;
    CHECK_PARSER_ERR(tx_display_query(displayIdx, tmpKey, sizeof(tmpKey), &ret_value_token_index))
    CHECK_APP_CANARY()
    snprintf(outKey, outKeyLen, "%s", tmpKey);

    bool is_default = false;

    if (parser_isAmount(tmpKey)) {
        CHECK_PARSER_ERR(parser_formatAmount(ret_value_token_index,
                                             outVal, outValLen,
                                             pageIdx, pageCount, &is_default))
    } else {
        CHECK_PARSER_ERR(tx_getToken(ret_value_token_index,
                                     outVal, outValLen,
                                     pageIdx, pageCount))
    }
    CHECK_APP_CANARY()

    CHECK_PARSER_ERR(tx_display_make_friendly())
    CHECK_APP_CANARY()
    // Display "Raw Amount" for non-default tokens
    if(memcmp(tmpKey, "Amount", 6) == 0 && is_default == false){
        snprintf(outKey, outKeyLen, "Raw %s", tmpKey);
    } else {
        snprintf(outKey, outKeyLen, "%s", tmpKey);
    }
    CHECK_APP_CANARY()

    return parser_ok;
}

parser_error_t parser_getItem(uint8_t displayIdx,
                              char *outKey, uint16_t outKeyLen,
                              char *outVal, uint16_t outValLen,
                              uint8_t pageIdx, uint8_t *pageCount) {

    CHECK_PARSER_ERR(parser_getJsonItem(displayIdx,
                        outKey, outKeyLen,
                        outVal, outValLen,
                        pageIdx, pageCount));

    return parser_ok;
}
