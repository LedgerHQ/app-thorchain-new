/** ******************************************************************************
 *  (c) 2021-2023 Zondax AG
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
 ******************************************************************************* */
import { DEFAULT_START_OPTIONS, IDeviceModel } from '@zondax/zemu'

const Resolve = require('path').resolve

export const AMINO_JSON_TX = 0x0
export const APP_SEED = 'equip will roof matter pink blind book anxiety banner elbow sun young'

const APP_PATH_S = Resolve('../app/output/app_s.elf')
const APP_PATH_X = Resolve('../app/output/app_x.elf')
const APP_PATH_SP = Resolve('../app/output/app_s2.elf')
const APP_PATH_ST = Resolve('../app/output/app_stax.elf')
const APP_PATH_FL = Resolve('../app/output/app_flex.elf')

export const defaultOptions = {
  ...DEFAULT_START_OPTIONS,
  logging: true,
  custom: `-s "${APP_SEED}"`,
  X11: false,
}

export const DEVICE_MODELS: IDeviceModel[] = [
  { name: 'nanos', prefix: 'S', path: APP_PATH_S },
  { name: 'nanox', prefix: 'X', path: APP_PATH_X },
  { name: 'nanosp', prefix: 'SP', path: APP_PATH_SP },
  { name: 'stax', prefix: 'ST', path: APP_PATH_ST },
  {name: 'flex', prefix: 'FL',  path: APP_PATH_FL},
]

export const example_tx_str_MsgSend = {
  "account_number": "588",
  "chain_id": "thorchain",
  "fee": {
      "amount": [],
      "gas": "2000000"
  },
  "memo": "TestMemo",
  "msgs": [
      {
          "type": "thorchain/MsgSend",
          "value": {
              "amount": [
                  {
                      "amount": "150000000",
                      "denom": "rune"
                  }
              ],
              "from_address": "tthor1c648xgpter9xffhmcqvs7lzd7hxh0prgv5t5gp",
              "to_address": "tthor10xgrknu44d83qr4s4uw56cqxg0hsev5e68lc9z"
          }
      }
  ],
  "sequence": "5"
};

export const example_tx_str_MsgDeposit = {
  "account_number": "588",
  "chain_id": "thorchain",
  "fee": {
      "amount": [],
      "gas": "10000000"
  },
  "memo": "",
  "msgs": [
      {
          "type": "thorchain/MsgDeposit",
          "value": {
              "coins": [
                  {
                      "amount": "330000000",
                      "asset": "THOR.RUNE"
                  }
              ],
              "memo": "SWAP:BNB.BNB:tbnb1qk2m905ypazwfau9cn0qnr4c4yxz63v9u9md20:",
              "signer": "tthor1c648xgpter9xffhmcqvs7lzd7hxh0prgv5t5gp"
          }
      }
  ],
  "sequence": "6"
};

export const example_tx_str_MsgDeposit_token_2 = {
    "account_number": "588",
    "chain_id": "thorchain",
    "fee": {
        "amount": [],
        "gas": "10000000"
    },
    "memo": "",
    "msgs": [
        {
            "type": "thorchain/MsgDeposit",
            "value": {
                "coins": [
                    {
                        "amount": "330000000",
                        "asset": "ETH.ETH"
                    }
                ],
                "memo": "SWAP:BNB.BNB:tbnb1qk2m905ypazwfau9cn0qnr4c4yxz63v9u9md20:",
                "signer": "tthor1c648xgpter9xffhmcqvs7lzd7hxh0prgv5t5gp"
            }
        }
    ],
    "sequence": "6"
};