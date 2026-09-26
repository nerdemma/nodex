#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../lib/blockchain.h"
#include "../lib/block.h"
#include "../lib/consensus.h"
#include "../lib/transaction.h"
#include "../lib/utils.h"


int blockchain_is_chain_valid(Block **blocks, size_t length, const ValidatorSet *val_set)
{
    if (!blocks || length == 0)
        return 0;

    for (size_t i = 0; i < length; i++)
    {
        Block *current = blocks[i];
        if (!current)
            return 0;

        if (!consensus_verify_block_seal(current, val_set))
        {
            fprintf(stderr, "[!] Reject: Firmas IBFT invalidas en el bloque %zu\n", i);
            return 0;
        }

        if (i > 0)
        {
            Block *previous = blocks[i - 1];
            if (memcmp(current->prev_hash, previous->hash, 32) != 0)
            {
                fprintf(stderr, "[!] Reject: Discrepancia en prev_hash en el bloque  %zu\n", i);
                return 0;
            }
        }
    }
    return 1;
}

int blockchain_resolve_fork(Blockchain *chain, Block **new_blocks, size_t new_length, const ValidatorSet *val_set)
{
    if (!chain || !new_blocks)
        return 0;

    if (new_length <= chain->length)
    {
        printf("[+] La cadena remota no es mas larga (%zu <= %zu) conservando cadena actual. \n", new_length, chain->length);
        return 0;
    }

    printf("[*] Evaluando resolucion de horquilla. Cadena local: %zu | Incoming chain: %zu...\n", chain->length, new_length);

    if (!blockchain_is_chain_valid(new_blocks, new_length, val_set))
    {
        fprintf(stderr, "[!] Fork Rejected: La cadena entrante contiene bloques invalidos. \n");
        return 0;
    }

    for (size_t i = 0; i < chain->length; i++)
    {
        if (chain->blocks[i])
        {
            block_free(chain->blocks[i]);
        }
    }

    free(chain->blocks);

    // Asignar nueva cadena
    chain->blocks = new_blocks;
    chain->length = new_length;

    // Persistir en el disco
    FILE *fp = fopen(CHAIN_FILE, "wb");

    if (fp)
    {
        for (size_t i = 0; i < chain->length; i++)
        {
            uint8_t buffer[1024];
            size_t bytes = block_serialize(chain->blocks[i], buffer);
            fwrite(buffer, 1, bytes, fp);
        }
        fclose(fp);
    }

    printf("[+] Fork resolved: Cadena local actualizada exitosamente a  %zu bloques \n", new_length);
    return 1;
}

Blockchain *blockchain_create(void)
{
    Blockchain *chain = (Blockchain *)malloc(sizeof(Blockchain));
    if (!chain)
        return NULL;

    Transaction genesis_txs[3];
    memset(genesis_txs, 0, sizeof(genesis_txs));

    transaction_create(&genesis_txs[0], "SYSTEM", "ALICE", (uint64_t)(1000.0 * 1000000.0), 0, 0, NULL);
    strncpy(genesis_txs[0].data, "Genesis allocation for ALICE", TX_DATA_LEN - 1);
    transaction_create(&genesis_txs[1], "SYSTEM", "BOB", (uint64_t)(1000.0 * 1000000.0), 0, 0, NULL);
    strncpy(genesis_txs[1].data, "Genesis allocation for BOB", TX_DATA_LEN - 1);
    transaction_create(&genesis_txs[2], "SYSTEM", "CARLIE", (uint64_t)(1000.0 * 1000000.0), 0, 0, NULL);
    strncpy(genesis_txs[2].data, "Genesis allocation for CARLIE", TX_DATA_LEN - 1);

    Block *genesis = block_create(NULL, genesis_txs, 3);
    if (!genesis)
    {
        free(chain);
        return NULL;
    }

    genesis->index = 0;
    genesis->timestamp = (uint32_t)time(NULL);
    genesis->round = 0;
    strncpy(genesis->proposer, "VAL_NODE_01", sizeof(genesis->proposer) - 1);
    genesis->commit_signatures_count = 1;
    strncpy(genesis->commit_signatures[0], "VAL_NODE_01", sizeof(genesis->commit_signatures[0]) - 1);
    block_calculate_hash(genesis);

    chain->capacity = 10;
    chain->blocks = (Block **)malloc(sizeof(Block *) * chain->capacity);
    chain->blocks[0] = genesis;
    chain->length = 1;
    memset(chain->filepath, 0, sizeof(chain->filepath));

    return chain;
}

int blockchain_add_block(Blockchain *chain, Block *new_block, const ValidatorSet *val_set)
{
    Block *prev_block = chain->blocks[chain->length - 1];
    if (memcmp(new_block->prev_hash, prev_block->hash, 32) != 0)
    {
        printf("[Error] El hash previo no coincide");
        return 0;
    }
    // verificar las firmas de compromiso de los validadores

    if (!consensus_verify_block_seal(new_block, val_set))
    {
        printf("[Error] El bloque no cumple con el quorum de firmas IBFT exigido. \n");
        return 0;
    }

    if (chain->length == chain->capacity)
    {
        chain->capacity *= 2;
        chain->blocks = (Block **)realloc(chain->blocks, sizeof(Block *) * chain->capacity);
    }

    chain->blocks[chain->length++] = new_block;
    return 1;
}

int blockchain_is_valid(const Blockchain *chain, const ValidatorSet *val_set)
{
    uint8_t recalculated[32];

    for (size_t i = 0; i < chain->length; i++)
    {
        Block *current = chain->blocks[i];

        // recalcular hash
        uint8_t buf[HEADER_SIZE];
        block_serialize_header(current, buf);
        double_sha256(buf, HEADER_SIZE, recalculated);
        if (memcmp(recalculated, current->hash, 32) != 0)
            return 0;

        // validar el hash

        if (i > 0)
        {
            Block *prev = chain->blocks[i - 1];
            if (memcmp(current->prev_hash, prev->hash, 32) != 0)
                return 0;
        }

        // verificar firmas de consenso
        if (!consensus_verify_block_seal(current, val_set))
            return 0;
    }
    return 1;
}

int blockchain_save_block(const Block *block, const char *filepath)
{
    FILE *file = fopen(filepath, "ab");
    if (!file)
        return -1;
    uint8_t buf[HEADER_SIZE];
    block_serialize_header(block, buf);
    size_t written = fwrite(buf, HEADER_SIZE, 1, file);
    fclose(file);
    return (written == 1) ? 0 : -1;
}


Blockchain* blockchain_init(const char *db_path)
{
	Blockchain *chain = storage_load_blockchain(db_path);

	if(chain != NULL)
	{
	printf("[STORAGE] Cadena cargada exitosamente desde :%s (Altura: %uint)\n", 
	db_path, chain->height);
	return chain;
	}

// si no existe el achivo se incializara desde cero
	printf("[STORAGE] No se encontró historia previa. Creando bloque Genesis...\n");
	chain = blockchain_create_empty();
	Block *genesis = create_genesis_block();
	
	blockchain_add_block(chain, genesis);
	storage_save_blockchain(db_path, chain);
return chain;
}
