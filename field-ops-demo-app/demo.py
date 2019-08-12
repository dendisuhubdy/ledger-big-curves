#!/usr/bin/env python
#*******************************************************************************
#*   Ledger Blue
#*   (c) 2016 Ledger
#*
#*  Licensed under the Apache License, Version 2.0 (the "License");
#*  you may not use this file except in compliance with the License.
#*  You may obtain a copy of the License at
#*
#*      http://www.apache.org/licenses/LICENSE-2.0
#*
#*  Unless required by applicable law or agreed to in writing, software
#*  distributed under the License is distributed on an "AS IS" BASIS,
#*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#*  See the License for the specific language governing permissions and
#*  limitations under the License.
#********************************************************************************
from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException
from secp256k1 import PublicKey



a = 0x7DA285E70863C79D56446237CE2E1468D14AE9BB64B2BB01B10E60A5D5DFE0A25714B7985993F62F03B22A9A3C737A1A1E0FCF2C43D7BF847957C34CCA1E3585F9A80A95F401867C4E80F4747FDE5ABA7505BA6FCF2485540B13DFC8468A
b = 0x7DA285E70863C79D56446237CE2E1468D14AE9BB64B2BB01B10E60A5D5DFE0A25714B7985993F62F03B22A9A3C737A1A1E0FCF2C43D7BF847957C34CCA1E3585F9A80A95F401867C4E80F4747FDE5ABA7505BA6FCF2485540B13DFC8468A
p = 0x1C4C62D92C41110229022EEE2CDADB7F997505B8FAFED5EB7E8F96C97D87307FDB925E8A0ED8D99D124D9A15AF79DB26C5C28C859A99B3EEBCA9429212636B9DFF97634993AA4D6C381BC3F0057974EA099170FA13A4FD90776E240000001

textToSign = ""
while True:
        data = raw_input("Enter text to sign, end with an empty line : ")
        if len(data) == 0:
                break
        textToSign += data + "\n"

dongle = getDongle(True)
publicKey = dongle.exchange(bytes("8004000000".decode('hex')))
print "publicKey " + str(publicKey).encode('hex')
try:
        offset = 0
        while offset <> len(textToSign):
                if (len(textToSign) - offset) > 255:
                        chunk = textToSign[offset : offset + 255]
                else:
                        chunk = textToSign[offset:]
                if (offset + len(chunk)) == len(textToSign):
                        p1 = 0x80
                else:
                        p1 = 0x00
                apdu = bytes("8002".decode('hex')) + chr(p1) + chr(0x00) + chr(len(chunk)) + bytes(chunk)
                signature = dongle.exchange(apdu)
                offset += len(chunk)
        print "signature " + str(signature).encode('hex')
        # publicKey = PublicKey(bytes(publicKey), raw=True)
        # signature = publicKey.ecdsa_deserialize(bytes(signature))
        print hex(a + b % p)
        print signature == a + b % p
        # print "verified " + str(publicKey.ecdsa_verify(bytes(textToSign), signature))
except CommException as comm:
        if comm.sw == 0x6985:
                print "Aborted by user"
        else:
                print "Invalid status " + comm.sw
