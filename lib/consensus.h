#ifndef CONSENSUS_H
#define CONSENSUS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "block.h"

#ifndef MAX_VALIDATORS
#define MAX_VALIDATORS 100
#endif

typedef struct Blockchain Blockchain;

typedef struct
{
char validators[MAX_VALIDATORS][32];
size_t count;
} ValidatorSet;

size_t consensus_max_faulty(size_t total_validators);
size_t consensus_quorum_size(size_t total_validators);
const char* consensus_get_proporser(const ValidatorSet *val_set, uint32_t block_index, uint32_t round);
bool consensus_verify_block_seal(const Block *block, const ValidatorSet *val_set);
int consensus_propose_block(Blockchain *chain, Block *block, const char *validator_addr);    
#endif

