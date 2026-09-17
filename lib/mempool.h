#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stddef.h>
#include "transaction.h"

#define MEMPOOL_FILE "mempool.dat"
#define MAX_MEMPOOL_TXS 1000

typedef struct{
    Transaction transactions[MAX_MEMPOOL_TXS];
    size_t count;
} Mempool;

void mempool_init(Mempool *mp);
int mempool_add_tx(Mempool *mp, const Transaction *tx);
void mempool_clear(Mempool *mp);

int mempool_save(const Mempool *mp, const char *filename);
int mempool_load(Mempool *mp, const char *filename);
void mempool_remove_tx(Mempool *mp, const Transaction *tx);

#endif //MEMPOOL_H
