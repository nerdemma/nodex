#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>
#include "blockchain.h"
#include "mempool.h"

void sha256(const uint8_t *data, size_t len, uint8_t out[32]);

void double_sha256(const uint8_t *data, size_t len, uint8_t out[32]);

void hex_encode(const uint8_t *in, size_t len, char *out);
void print_hash(const uint8_t *hash, size_t  len);
void print_blockchain_json(const Blockchain *chain);
void print_mempool_json(const Mempool *mp);

#endif // UTILS_H


