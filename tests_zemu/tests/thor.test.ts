/** ******************************************************************************
 *  (c) 2018-2022 Zondax GmbH
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

import Zemu, { ClickNavigation, TouchNavigation, IDeviceModel } from '@zondax/zemu'
// @ts-ignore
import { ThorchainApp } from '@blooo/ledger-thorchain-js'
import {
  defaultOptions,
  DEVICE_MODELS,
  AMINO_JSON_TX,
  example_tx_str_MsgSend,
  example_tx_str_MsgDeposit,
  example_tx_str_MsgDeposit_token_2
} from './common'

// @ts-ignore
import secp256k1 from 'secp256k1/elliptic'
// @ts-ignore
import crypto from 'crypto'
import { ButtonKind, IButton } from '@zondax/zemu/dist/types'

jest.setTimeout(120000)

async function signAndVerifyTransaction(m: IDeviceModel, test_name: String, transaction: any) {
  const sim = new Zemu(m.path);
  try {
    await sim.start({ ...defaultOptions, model: m.name });
    const app = new ThorchainApp(sim.getTransport());

    const path = [44, 931, 0, 0, 0];
    const tx = Buffer.from(JSON.stringify(transaction), "utf-8");
    const hrp = 'thor';

    // get address / publickey
    const respPk = await app.getAddressAndPubKey(path, hrp);
    expect(respPk.return_code).toEqual(0x9000)
    expect(respPk.error_message).toEqual('No errors')
    console.log(respPk);

    // do not wait here..
    const signatureRequest = app.sign(path, tx, AMINO_JSON_TX);

    // Wait until we are not in the main menu
    await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());
    await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-${test_name}`);

    const resp = await signatureRequest;
    console.log(resp);

    expect(resp.return_code).toEqual(0x9000)
    expect(resp.error_message).toEqual('No errors')
    expect(resp).toHaveProperty('signature')

    // Now verify the signature
    const hash = crypto.createHash('sha256');
    const msgHash = Uint8Array.from(hash.update(tx).digest());

    const signatureDER = resp.signature;
    const signature = secp256k1.signatureImport(Uint8Array.from(signatureDER));

    const pk = Uint8Array.from(respPk.compressed_pk);

    const signatureOk = secp256k1.ecdsaVerify(signature, msgHash, pk);
    expect(signatureOk).toEqual(true);
  } finally {
    await sim.close();
  }
}

describe('Thor', function () {
  // eslint-disable-next-line jest/expect-expect
  test.concurrent.each([DEVICE_MODELS])('can start and stop container', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
    } finally {
      await sim.close()
    }
  });

  test.concurrent.each(DEVICE_MODELS)('sign msgSend normal', async function (m) {
    await signAndVerifyTransaction(m, 'sign_msgSend', example_tx_str_MsgSend);
  });

  test.concurrent.each(DEVICE_MODELS)('sign msgDeposit normal', async function (m) {
    await signAndVerifyTransaction(m, 'sign_msgDeposit', example_tx_str_MsgDeposit);
  });

  test.concurrent.each(DEVICE_MODELS)('sign msgDeposit token 2', async function (m) {
    await signAndVerifyTransaction(m, 'sign_msgDeposit_token_2', example_tx_str_MsgDeposit_token_2);
  });

})
