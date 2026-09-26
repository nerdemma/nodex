#include "../lib/transaction.h"
#include "../lib/utils.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <openssl/hmac.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <openssl/err.h>

static void bytes_to_hex(const uint8_t *in, size_t len, char *out)
{
    for (size_t i = 0; i < len; i++)
    {
        sprintf(out + (i * 2), "%02x", in[i]);
    }
    out[len * 2] = '\0';
}

static int hex_to_bytes(const char *hex, uint8_t *out, size_t out_len)
{
    size_t hexlen = strlen(hex);
    if (hexlen % 2 != 0) return -1;
    size_t bytes_len = hexlen / 2;
    if (bytes_len > out_len) return -1;
    for (size_t i = 0; i < bytes_len; i++)
    {
        unsigned int v;
        if (sscanf(hex + (i * 2), "%2x", &v) != 1) return -1;
        out[i] = (uint8_t)v;
    }
    return (int)bytes_len;
}

void transaction_create(Transaction *tx, const char *sender, const char *receiver, uint64_t amount, uint64_t fee, uint64_t nonce, const char *pubkey)
{
    if (!tx) return;
    memset(tx, 0, sizeof(Transaction));

    if (sender) { strncpy(tx->sender, sender, sizeof(tx->sender) - 1); }
    if (receiver) { strncpy(tx->receiver, receiver, sizeof(tx->receiver) - 1); }

    tx->amount = amount;
    tx->fee = fee;
    tx->nonce = nonce;
    tx->timestamp = (uint64_t)time(NULL);
    if (pubkey) strncpy(tx->pubkey, pubkey, sizeof(tx->pubkey) - 1);

    // calcular hash
    transaction_calculate_hash(tx, tx->tx_hash);
}

void transaction_calculate_hash(const Transaction *tx, uint8_t out_hash[32])
{
    if (!tx || !out_hash) return;

    // Deterministic serialization
    uint8_t buffer[1024];
    size_t offset = 0;

    memcpy(buffer + offset, tx->sender, sizeof(tx->sender));
    offset += sizeof(tx->sender);
    memcpy(buffer + offset, tx->receiver, sizeof(tx->receiver));
    offset += sizeof(tx->receiver);
    memcpy(buffer + offset, &tx->amount, sizeof(tx->amount));
    offset += sizeof(tx->amount);
    memcpy(buffer + offset, &tx->fee, sizeof(tx->fee));
    offset += sizeof(tx->fee);
    memcpy(buffer + offset, &tx->nonce, sizeof(tx->nonce));
    offset += sizeof(tx->nonce);
    memcpy(buffer + offset, &tx->timestamp, sizeof(tx->timestamp));
    offset += sizeof(tx->timestamp);
    memcpy(buffer + offset, tx->data, sizeof(tx->data));
    offset += sizeof(tx->data);

    double_sha256(buffer, offset, out_hash);
}

int transaction_sign(Transaction *tx, const char *key_hex)
{
    if (!tx) return -1;
    // snapshot fields for debug
    char snap_sender[33] = {0};
    char snap_receiver[33] = {0};
    uint64_t snap_amount = 0, snap_fee = 0, snap_nonce = 0, snap_timestamp = 0;
    strncpy(snap_sender, tx->sender, 32);
    strncpy(snap_receiver, tx->receiver, 32);
    snap_amount = tx->amount; snap_fee = tx->fee; snap_nonce = tx->nonce; snap_timestamp = tx->timestamp;
    // ECDSA path: require a 32-byte private key in hex
    uint8_t priv_bytes[64];
    if (!key_hex || strlen(key_hex) == 0) return -1;
    // sanitize key_hex: remove non-hex characters (colons, spaces)
    char clean_hex[129];
    size_t ci = 0;
    for (size_t i = 0; i < strlen(key_hex) && ci + 1 < sizeof(clean_hex); i++) {
        char c = key_hex[i];
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
            clean_hex[ci++] = c;
        }
    }
    clean_hex[ci] = '\0';
    int priv_len = hex_to_bytes(clean_hex, priv_bytes, sizeof(priv_bytes));
    if (priv_len != 32) return -1;
        // Use secp256k1
        int ret = -1;
        EC_KEY *eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (!eckey) return -1;

        BIGNUM *priv_bn = BN_bin2bn(priv_bytes, priv_len, NULL);
        if (!priv_bn) { EC_KEY_free(eckey); fprintf(stderr, "[!] BN_bin2bn failed\n"); return -1; }
        if (EC_KEY_set_private_key(eckey, priv_bn) != 1) { unsigned long e = ERR_get_error(); fprintf(stderr, "[!] EC_KEY_set_private_key failed: %s\n", ERR_error_string(e, NULL)); BN_free(priv_bn); EC_KEY_free(eckey); return -1; }

        // derive public key
        const EC_GROUP *group = EC_KEY_get0_group(eckey);
        EC_POINT *pub_point = EC_POINT_new(group);
        if (!pub_point) { BN_free(priv_bn); EC_KEY_free(eckey); return -1; }
        if (EC_POINT_mul(group, pub_point, priv_bn, NULL, NULL, NULL) != 1) { unsigned long e = ERR_get_error(); fprintf(stderr, "[!] EC_POINT_mul failed: %s\n", ERR_error_string(e, NULL)); EC_POINT_free(pub_point); BN_free(priv_bn); EC_KEY_free(eckey); return -1; }
        if (EC_KEY_set_public_key(eckey, pub_point) != 1) { unsigned long e = ERR_get_error(); fprintf(stderr, "[!] EC_KEY_set_public_key failed: %s\n", ERR_error_string(e, NULL)); EC_POINT_free(pub_point); BN_free(priv_bn); EC_KEY_free(eckey); return -1; }

        // sign tx hash
        unsigned int sig_len = ECDSA_size(eckey);
        unsigned char *sig = OPENSSL_malloc(sig_len);
        if (!sig) { EC_POINT_free(pub_point); BN_free(priv_bn); EC_KEY_free(eckey); return -1; }
        if (ECDSA_sign(0, tx->tx_hash, 32, sig, &sig_len, eckey) != 1) {
            unsigned long e = ERR_get_error(); fprintf(stderr, "[!] ECDSA_sign failed: %s\n", ERR_error_string(e, NULL)); OPENSSL_free(sig); EC_POINT_free(pub_point); BN_free(priv_bn); EC_KEY_free(eckey); return -1;
        }

        // store signature as hex
        bytes_to_hex(sig, sig_len, tx->signature);

        // store public key as DER hex
        unsigned char *pub_der = NULL;
        int pub_der_len = i2o_ECPublicKey(eckey, &pub_der);
        if (pub_der_len > 0 && pub_der) {
            bytes_to_hex(pub_der, pub_der_len, tx->pubkey);
            OPENSSL_free(pub_der);
        }

        OPENSSL_free(sig);
        // verify tx hash unchanged
        uint8_t check_hash_now[32]; transaction_calculate_hash(tx, check_hash_now);
        EC_POINT_free(pub_point);
        BN_free(priv_bn);
        // sanity: ensure tx hash unchanged after signing
        uint8_t check_hash[32];
        transaction_calculate_hash(tx, check_hash);
        if (memcmp(check_hash, tx->tx_hash, 32) != 0) {
            fprintf(stderr, "[!] transaction_sign altered tx hash\n");
        }

        EC_KEY_free(eckey);

        return 0;
}

int transaction_is_valid(const Transaction *tx)
{
    if (!tx) return 0;
    if (tx->amount == 0) return 0;
    if (strlen(tx->sender) == 0 || strlen(tx->receiver) == 0) return 0;

    uint8_t calc_hash[32];
    transaction_calculate_hash(tx, calc_hash);
    if (memcmp(calc_hash, tx->tx_hash, 32) != 0)
    {
        fprintf(stderr, "[!] tx hash mismatch\n");
        fprintf(stderr, " calc: ");
        for (int i = 0; i < 32; i++) fprintf(stderr, "%02x", calc_hash[i]);
        fprintf(stderr, "\n");
        fprintf(stderr, " store: ");
        for (int i = 0; i < 32; i++) fprintf(stderr, "%02x", tx->tx_hash[i]);
        fprintf(stderr, "\n");
        return 0;
    }

    // Allow SYSTEM sender to bypass signature for genesis and system txs
    if (strcmp(tx->sender, "SYSTEM") == 0) return 1;

    // Require pubkey (DER hex) and perform ECDSA verification only
    if (strlen(tx->pubkey) == 0) return 0;
    uint8_t pub_bytes[512];
    int pub_len = hex_to_bytes(tx->pubkey, pub_bytes, sizeof(pub_bytes));
    if (pub_len <= 0) return 0;
    // Reconstruct EC_KEY from octet public key using EC_POINT_oct2point for robustness
    EC_GROUP *group = NULL;
    EC_POINT *point = NULL;
    EC_KEY *pubkey = NULL;
    group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    if (!group) { EC_GROUP_free(group); return 0; }
    point = EC_POINT_new(group);
    if (!point) { EC_GROUP_free(group); return 0; }
    if (EC_POINT_oct2point(group, point, pub_bytes, pub_len, NULL) != 1) { EC_POINT_free(point); EC_GROUP_free(group); return 0; }
    pubkey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!pubkey) { EC_POINT_free(point); EC_GROUP_free(group); return 0; }
    if (EC_KEY_set_public_key(pubkey, point) != 1) { EC_KEY_free(pubkey); EC_POINT_free(point); EC_GROUP_free(group); return 0; }
    EC_POINT_free(point);
    EC_GROUP_free(group);

    uint8_t sig_bytes[512];
    int sig_len = hex_to_bytes(tx->signature, sig_bytes, sizeof(sig_bytes));
    if (sig_len <= 0) { EC_KEY_free(pubkey); return 0; }

    int ok = ECDSA_verify(0, tx->tx_hash, 32, sig_bytes, sig_len, pubkey);
    if (ok != 1) {
        unsigned long e = ERR_get_error();
        fprintf(stderr, "[!] ECDSA_verify failed (pub_len=%d sig_len=%d) err=%s\n", pub_len, sig_len, ERR_error_string(e, NULL));
    }
    EC_KEY_free(pubkey);
    return ok == 1;
}

size_t transaction_serialize(const Transaction *tx, uint8_t *buffer)
{
    if (!tx || !buffer) return 0;
    memcpy(buffer, tx, sizeof(Transaction));
    return sizeof(Transaction);
}

size_t transaction_deserialize(const uint8_t *buffer, Transaction *tx)
{
    if (!tx || !buffer) return 0;
    memcpy(tx, buffer, sizeof(Transaction));
    return sizeof(Transaction);
}
