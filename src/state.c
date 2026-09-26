#include <stdio.h>
#include <string.h>
#include "../lib/state.h"

// Return balances as double (converted from micro-units)
static double micro_to_double(uint64_t micro)
{
    return (double)micro / 1000000.0;
}

double state_get_balance(const Blockchain *chain, const char *address)
{
    if(!chain || !address) return 0.0;
    uint64_t balance = 0;

    for(size_t i=0; i < chain->length; i++)
    {
        Block *b = chain->blocks[i];
        if(!b) continue;
        for(uint32_t j=0; j < b->tx_count; j++)
        {
            Transaction *tx = &b->transactions[j];

            if(strcmp(tx->receiver, address) == 0)
            {
                balance += tx->amount;
            }

            if (strcmp(tx->sender, address) == 0)
            {
                balance -= (tx->amount + tx->fee);
            }
        }
    }

    return micro_to_double(balance);
}

double state_get_effective_balance(const Blockchain *chain, const Mempool *mp, const char *address)
{
    double balance = state_get_balance(chain, address);

    if(!mp) return balance;

    uint64_t diff = 0;
    for(size_t i = 0; i < mp->count; i++)
    {
        const Transaction *tx = &mp->transactions[i];
        if (strcmp(tx->sender, address) == 0)
        {
            diff += (tx->amount + tx->fee);
        }
    }

    return balance - micro_to_double(diff);
}

int state_validate_tx(const Blockchain *chain, const Mempool *mp, const Transaction *tx)
{
    if(!tx || tx->amount == 0) return 0;

    if(strcmp(tx->sender,"SYSTEM") == 0 || strcmp(tx->sender, "0x00") == 0) {return 1;}

    // Ensure tx is structurally valid (signature/hash)
    if (!transaction_is_valid(tx)) return 0;

    // Nonce/replay protection: determine highest seen nonce for sender in chain and mempool
    uint64_t highest_nonce = 0;
    if (chain) {
        for (size_t i = 0; i < chain->length; i++) {
            Block *b = chain->blocks[i];
            if (!b) continue;
            for (uint32_t j = 0; j < b->tx_count; j++) {
                Transaction *ctx = &b->transactions[j];
                if (strcmp(ctx->sender, tx->sender) == 0) {
                    if (ctx->nonce > highest_nonce) highest_nonce = ctx->nonce;
                }
            }
        }
    }
    if (mp) {
        for (size_t i = 0; i < mp->count; i++) {
            const Transaction *m = &mp->transactions[i];
            if (strcmp(m->sender, tx->sender) == 0) {
                if (m->nonce > highest_nonce) highest_nonce = m->nonce;
            }
        }
    }

    // tx nonce must be strictly greater than highest seen nonce
    if (tx->nonce <= highest_nonce) return 0;

    double available = state_get_effective_balance(chain, mp, tx->sender);
    double required = micro_to_double(tx->amount + tx->fee);

    return (available >= required) ? 1 : 0;
}
