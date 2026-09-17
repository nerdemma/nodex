#include "../lib/consensus.h"
#include "../lib/blockchain.h"
#include <string.h>
#include <stdio.h>

size_t consensus_max_faulty(size_t total_validators)
{
if(total_validators < 1) return 0;
return (total_validators -1) / 3;
}

size_t consensus_quorum_size(size_t total_validators)
{
size_t f = consensus_max_faulty(total_validators);
return (2 * f) + 1; 
}

const char * consensus_get_proporser(const ValidatorSet *val_set, uint32_t block_index, uint32_t round)
{
if(!val_set || val_set->count == 0) return NULL;

size_t index = (block_index + round) % val_set -> count;
return val_set -> validators[index];
}


bool consensus_verify_block_seal(const Block *block, const ValidatorSet *val_set)
{
if(!block || !val_set) return false;

size_t required_signatures = consensus_quorum_size(val_set->count);
uint32_t valid_signatures = block->commit_signatures_count;

if(valid_signatures < required_signatures)
    {
    printf("Consenso rechazado: firmas insuficientes (%u/%zu requeridas)\n",
    valid_signatures, required_signatures);

return false;
    }
return true;
}


int consensus_propose_block(Blockchain *chain, Block *block, const char *validator_addr)
{
if (!chain || !block || !validator_addr) { return 0;}
if (strlen(validator_addr) == 0) { return 0;}
return 1;
}
