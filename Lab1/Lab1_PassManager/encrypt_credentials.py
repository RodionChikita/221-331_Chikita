#!/usr/bin/env python3
"""
Utility to prepare the encrypted credentials file for Lab1_PassManager.

Steps:
1. Reads credentials_plain.json
2. Encrypts each login/password with layer-2 AES-256-CBC (PBKDF2 key from PIN + LAYER2_SALT)
3. Builds a new JSON with url (plaintext), encrypted_login, encrypted_password (base64)
4. Encrypts the entire JSON with layer-1 AES-256-CBC (PBKDF2 key from PIN + LAYER1_SALT)
5. Writes IV + ciphertext to credentials.json.enc
"""

import json
import hashlib
import os
import sys
import base64

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives import padding as sym_padding
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.backends import default_backend

KEY_LEN = 32
IV_LEN = 16
PBKDF2_ITERATIONS = 100000

LAYER1_SALT = bytes.fromhex("a1b2c3d4e5f60718")
LAYER2_SALT = bytes.fromhex("18070605d4c3b2a1")
LAYER2_IV   = bytes.fromhex("00112233445566778899aabbccddeeff")


def derive_key(pin: str, salt: bytes) -> bytes:
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=KEY_LEN,
        salt=salt,
        iterations=PBKDF2_ITERATIONS,
        backend=default_backend()
    )
    return kdf.derive(pin.encode("utf-8"))


def aes_encrypt(plaintext: bytes, key: bytes, iv: bytes) -> bytes:
    padder = sym_padding.PKCS7(128).padder()
    padded = padder.update(plaintext) + padder.finalize()

    cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=default_backend())
    enc = cipher.encryptor()
    return enc.update(padded) + enc.finalize()


def main():
    if len(sys.argv) < 2:
        print("Usage: python encrypt_credentials.py <PIN>")
        sys.exit(1)

    pin = sys.argv[1]

    with open("credentials_plain.json", "r", encoding="utf-8") as f:
        creds = json.load(f)

    layer2_key = derive_key(pin, LAYER2_SALT)

    two_layer_creds = []
    for entry in creds:
        enc_login = aes_encrypt(entry["login"].encode("utf-8"), layer2_key, LAYER2_IV)
        enc_pass  = aes_encrypt(entry["password"].encode("utf-8"), layer2_key, LAYER2_IV)

        two_layer_creds.append({
            "url": entry["url"],
            "encrypted_login": base64.b64encode(enc_login).decode("ascii"),
            "encrypted_password": base64.b64encode(enc_pass).decode("ascii")
        })

    inner_json = json.dumps(two_layer_creds, ensure_ascii=False, indent=2).encode("utf-8")
    print(f"Inner JSON size: {len(inner_json)} bytes")

    layer1_key = derive_key(pin, LAYER1_SALT)
    iv = os.urandom(IV_LEN)

    ciphertext = aes_encrypt(inner_json, layer1_key, iv)

    with open("credentials.json.enc", "wb") as f:
        f.write(iv + ciphertext)

    print(f"Written credentials.json.enc ({IV_LEN + len(ciphertext)} bytes)")
    print("Done!")


if __name__ == "__main__":
    main()
