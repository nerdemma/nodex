#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <stddef.h>
#include <stdint.h>
#include "block.h"
#include "transaction.h"
#include "consensus.h"

#define CHAIN_FILE "blockchain.db"


typedef struct Blockchain {
    Block **blocks;
    size_t length;
    size_t capacity;
    char filepath[256];
} Blockchain;


int         blockchain_is_chain_valid(Block **blocks, size_t lenghth, const ValidatorSet *val_set);
Blockchain* blockchain_create(void);
Blockchain* blockchain_init(const char *filepath);
int         blockchain_add_block(Blockchain *chain, Block *new_block, const ValidatorSet *val_set);
int         blockchain_is_valid(const Blockchain *chain, const ValidatorSet *val_set);
int         blockchain_resolve_fork(Blockchain *chain, Block **new_blocks, size_t new_length, const ValidatorSet *val_set);
void        blockchain_free(Blockchain *chain);
int         blockchain_save_block(const Block *block, const char *filepath);
int         blockchain_write_to_file(Blockchain *chain, const char *filename);

#endif
