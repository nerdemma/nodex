#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "../lib/blockchain.h"
#include "../lib/net.h"
#include "../lib/mempool.h"
#include "../lib/discovery.h"
#include "../lib/peer_pool.h"
#include "../lib/p2p_sync.h"
#include "../lib/storage.h"

typedef struct NodeDaemon {
    Mempool *mempool;
    Blockchain *chain;
    const char *db_path;
    pthread_mutex_t mempool_mutex;
    pthread_mutex_t chain_mutex;
    void *consensus;
} NodeDaemon;

static volatile int daemon_running = 1;

void *validator_loop(void *arg)
{
    NodeDaemon *daemon = (NodeDaemon *)arg;
    const int BLOCK_TIME_SECONDS = 5;

    if (!daemon)
        return NULL;

    printf("[VALIDATOR] Loop de produccion de bloques iniciando\n");

    while (daemon_running)
    {
        sleep(BLOCK_TIME_SECONDS);

        pthread_mutex_lock(&daemon->mempool_mutex);
        size_t pending_txs = mempool_size(daemon->mempool);
        pthread_mutex_unlock(&daemon->mempool_mutex);

        if (pending_txs == 0)
            continue;

        printf("[VALIDATOR] Transacciones encontradas (%zu). Proponiendo nuevo bloque\n", pending_txs);

        pthread_mutex_lock(&daemon->chain_mutex);
        if (daemon->mempool && daemon->chain && daemon->mempool->count > 0)
        {
            uint32_t tx_count = (daemon->mempool->count > MAX_TX_PER_BLOCK) ? MAX_TX_PER_BLOCK : (uint32_t)daemon->mempool->count;
            Block *new_block = block_create(NULL, daemon->mempool->transactions, tx_count);

            if (new_block != NULL)
            {
                ValidatorSet val_set = {0};
                val_set.count = 1;
                strncpy(val_set.validators[0], "VAL_NODE_01", sizeof(val_set.validators[0]) - 1);

                if (blockchain_add_block(daemon->chain, new_block, &val_set))
                {
                    storage_save_blockchain(daemon->chain, daemon->db_path);
                    printf("[VALIDATOR] Bloque forjado exitosamente. Altura: %zu\n", daemon->chain->length);
                }
                else
                {
                    block_free(new_block);
                }
            }
        }
        pthread_mutex_unlock(&daemon->chain_mutex);
    }

    return NULL;
}

int node_daemon_start(NodeDaemon *daemon)
{
    pthread_t validator_thread;

    if (!daemon)
        return -1;

    if (pthread_create(&validator_thread, NULL, validator_loop, (void *)daemon) != 0)
    {
        fprintf(stderr, "[ERROR] No se pudo crear el hilo del validador\n");
        return -1;
    }

    pthread_detach(validator_thread);
    return 0;
}

int main(int argc, char *argv[])
{
    uint16_t tcp_port = DEFAULT_PORT;

    if (argc > 1)
    {
        tcp_port = (uint16_t)atoi(argv[1]);
    }

    peer_pool_init(&g_peer_pool);
    Blockchain *chain = blockchain_init(CHAIN_FILE);

    if (!chain)
    {
        fprintf(stderr, "[Critical Error] Blockchain not initializng.\n");
        return 1;
    }

    Mempool mp;
    mempool_init(&mp);
    mempool_load(&mp, MEMPOOL_FILE);
    printf("[Initialing] Demon P2P. \n[Mempool] Loaded with %zu transactions.\n", mp.count);
    int server_fd = start_server(tcp_port);
    discovery_start(tcp_port);

    if (p2p_sync_start(server_fd, chain, &mp, &g_peer_pool) == 0)
    {
        printf("[Demon P2P] Running in port:%d. Press Ctrl+C to exit \n", tcp_port);
    }

    while (1)
    {
        sleep(1);
    }

    p2p_sync_stop();
    if (server_fd >= 0)
        close(server_fd);
    mempool_clear(&mp);
    blockchain_free(chain);
    return 0;
}
