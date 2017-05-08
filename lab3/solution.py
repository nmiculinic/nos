import argparse
from Crypto.Cipher import AES
from Crypto import Random
from Crypto.PublicKey import RSA
import logging
from Crypto.Hash import SHA512
import struct
from Crypto.Signature import PKCS1_v1_5
random_generator = Random.new().read

log_fmt = '\r[%(levelname)s] %(name)s: %(message)s'
logging.basicConfig(level=logging.INFO, format=log_fmt)

parser = argparse.ArgumentParser()

subparsers = parser.add_subparsers(dest='operation')
subparsers.required = True

gen_parser = subparsers.add_parser('gen', help="generate keys")
gen_parser.add_argument("--type", choices=["symetric", "asymetric"], default="symetric")
gen_parser.add_argument("keyfile")

encrypt_parser = subparsers.add_parser("encrypt")
decrypt_parser = subparsers.add_parser("decrypt")

for p in [encrypt_parser, decrypt_parser]:
    p.add_argument("--keyfile", required=True)
    p.add_argument("--sign_key")
    p.add_argument("input_file")
    p.add_argument("output_file")

sign_parser = subparsers.add_parser("sign")
verify_parser = subparsers.add_parser("verify")

for p in [sign_parser, verify_parser]:
    p.add_argument("--sign_key", required=True)
    p.add_argument("input_file")
    p.add_argument("signature_file")

args = parser.parse_args()


def sign(message, key):
    h = SHA512.new(message)
    signer = PKCS1_v1_5.new(key)
    signature = signer.sign(h)
    return signature


def verify(message, key, signature):
    h = SHA512.new(message)
    verifier = PKCS1_v1_5.new(key)
    if verifier.verify(h, signature):
        logging.info("Signature is genuine")
    else:
        logging.error("Signature mismatch")
        raise ValueError()


def pad_data(data, block_size):
    length = block_size - (len(data) % block_size)
    data += Random.get_random_bytes(length - 1)
    data += bytes([length])
    return data


def unpad_data(data, block_size):
    pad_length = int(data[-1])
    return data[:-pad_length]


if args.operation == "gen":
    if args.type == "symetric":
        key = Random.get_random_bytes(AES.block_size)
        with open(args.keyfile, 'wb') as f:
            f.write(key)
            print(repr(key))
        logging.info("Outputed key to %s", args.keyfile)
    elif args.type == "asymetric":
        key = RSA.generate(1024, random_generator)
        with open(args.keyfile, 'wb') as f:
            f.write(key.exportKey())
        logging.info("Outputed key to %s in PEM format", args.keyfile)
elif args.operation in ["sign", "verify"]:
    with open(args.sign_key, 'rb') as f:
        key = RSA.importKey(f.read())
    with open(args.input_file, 'rb') as f:
        message = f.read()
        signature = sign(message, key)

    if args.operation == "sign":
        with open(args.signature_file, 'wb') as f:
            f.write(signature)
    else:
        with open(args.signature_file, 'rb') as f:
            verify(message, key, f.read())
elif args.operation in ["encrypt", "decrypt"]:
    AES_IV = b'\xaeiO\xfc\x00\x95\x1d\x8c!\xc8rp\x87\x86O\x08'
    with open(args.keyfile, 'rb') as f:
        key = f.read()

    if args.sign_key is not None:
        with open(args.sign_key, 'rb') as f:
            sign_key = RSA.importKey(f.read())
    else:
        sign_key = None

    cypher = AES.new(key, AES.MODE_CFB, AES_IV)

    if args.operation == "encrypt":
        with open(args.input_file, 'rb') as f:
            message = f.read()
            message = pad_data(message, cypher.block_size)

        if sign_key is not None:
            with open(args.sign_key, 'rb') as f:
                signature = sign(message, sign_key)
                message += signature
                message += struct.pack("!h", len(signature))
                message = pad_data(message, cypher.block_size)

        with open(args.output_file, 'wb') as f:
            f.write(cypher.encrypt(message))
            logging.info("Successfuly encrypted file to %s", args.output_file)

    else:
        with open(args.input_file, 'rb') as f:
            message = f.read()
            message = cypher.decrypt(message)
            message = unpad_data(message, cypher.block_size)

        if sign_key is not None:
            with open(args.sign_key, 'rb') as f:
                sign_len = struct.unpack("!h", message[-2:])[0]
                message = message[:-2]
                signature = message[-sign_len:]
                message = message[:-sign_len]
                verify(message, sign_key, signature)
                message = unpad_data(message, cypher.block_size)

        with open(args.output_file, 'wb') as f:
            f.write(message)
            logging.info("Successfuly decrypted file to %s", args.output_file)
