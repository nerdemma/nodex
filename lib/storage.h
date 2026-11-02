#ifndef STORAGE_H
#define STORAGE_H

#include "blockchain.h"

Blockchain *storage_load_blockchain(const char *filepath);
int storage_save_blockchain(const Blockchain *chain, const char *filepath);

#endif // STORAGE_H
