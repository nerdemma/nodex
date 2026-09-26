#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <stdint.h>
#include <stddef.h>

#define ADDRESS_LEN 65
#define TX_DATA_LEN 128

// Amounts are represented as integer micro-units (1 unit = 1e-6)
typedef struct {
    char sender[32];
    char receiver[32];
    uint64_t amount; // micro-units
    uint64_t fee;    // micro-units
    uint64_t nonce;
    uint64_t timestamp;
    uint8_t tx_hash[32];
    // pubkey and signature stored as hex strings for simplicity in MVP
    char pubkey[513]; // hex (allow larger DER hex)
    char signature[513]; // hex (allow larger DER hex)
    char data[TX_DATA_LEN];
} Transaction;

void transaction_create(Transaction *tx, const char *sender, const char *receiver, uint64_t amount, uint64_t fee, uint64_t nonce, const char *pubkey);
void transaction_calculate_hash(const Transaction *tx, uint8_t out_hash[32]);
int  transaction_is_valid(const Transaction *tx);
int  transaction_sign(Transaction *tx, const char *key_hex);

size_t transaction_serialize(const Transaction *tx, uint8_t *buffer);
size_t transaction_deserialize(const uint8_t *buffer, Transaction *tx);

#endif // TRANSACTION_H
