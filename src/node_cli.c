#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <stdint.h>

#include "../lib/blockchain.h"
#include "../lib/mempool.h"
#include "../lib/block.h"
#include "../lib/peer_pool.h"
#include "../lib/net.h"
#include "../lib/state.h"
#include "../lib/consensus.h"
#include "../lib/storage.h"
#include "../lib/utils.h"
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/bn.h>
#include <sys/stat.h>

ValidatorSet val_set;

static void init_validator_set(void)
{
    memset(&val_set, 0, sizeof(val_set));
    strncpy(val_set.validators[0], "VAL_NODE_01", sizeof(val_set.validators[0]) - 1);
    val_set.count = 1;
}


void print_help(const char *prog_name)
{
    printf("Uso: %s [OPCIONES]\n", prog_name);
    printf(" -s --status                Ver el estado e integridad de la cadena local\n");
    printf(" -S --status-json           Salida JSON del estado de la cadena\n");
    printf(" -t --tx<from:to:amount>    Crear una nueva trasaccion de prueba (ej: ALICE:BOB:25.5)\n");
    printf(" -k --key <hex_priv>         Clave privada ECDSA en hex para firmar transacciones\n");
    printf(" -c --connect <ip:port>     Connectar a un peer remoto (ej:192.168.0.2:8333)\n");
    printf(" -b --balance <address>     Consultar el saldo de una cuenta \n");
    printf(" -B --balance-json <address> Salida JSON del saldo de una cuenta\n");
    printf(" -M --mempool-json          Salida JSON de la mempool actual\n");
    printf(" -p --propose <validator>   Proponer/validar un nuevo bloque de consenso \n");
    printf(" -h --help                  Mostrar esta ayuda.\n");

}

int main(int argc, char *argv[])
{
    if(argc < 2) { print_help(argv[0]); return 0; }

    peer_pool_init(&g_peer_pool);
    init_validator_set();
    Blockchain *chain = blockchain_init(CHAIN_FILE);
    if(!chain) { fprintf(stderr, "[!] Error: no se pudo cargar la blockchain desde el disco.\n"); return 1; }

    Mempool mp;
    mempool_init(&mp);

    static struct option long_options[] = {
        {"status",       no_argument,        0,  's'},
        {"status-json",  no_argument,        0,  'S'},
        {"tx",           required_argument,  0,  't'},
        {"key",          required_argument,  0,  'k'},
        {"keygen",       no_argument,        0,  'g'},
        {"connect",      required_argument,  0,  'c'},
        {"balance",      required_argument,  0,  'b'},
        {"balance-json", required_argument,  0,  'B'},
        {"mempool-json", no_argument,        0,  'M'},
        {"propose",      required_argument,  0,  'p'},
        {"help",         no_argument,        0,  'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    char privkey_hex[129] = {0};

    while ((opt = getopt_long(argc, argv, "sSt:k:g:c:b:B:M:p:h", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'g':
            {
                // generate secp256k1 keypair and store private PEM under ~/.config/blockchain/keys
                const char *home = getenv("HOME");
                char dirpath[512];
                if (!home) home = "/tmp";
                snprintf(dirpath, sizeof(dirpath), "%s/.config/blockchain/keys", home);
                // create directories if needed
                char cmd[1024];
                snprintf(cmd, sizeof(cmd), "mkdir -p %s && chmod 700 %s", dirpath, dirpath);
                system(cmd);

                EC_KEY *eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
                if (!eckey) { fprintf(stderr, "[!] Error: no se pudo inicializar EC key.\n"); break; }
                if (EC_KEY_generate_key(eckey) != 1) { EC_KEY_free(eckey); fprintf(stderr, "[!] Error: fallo al generar clave EC.\n"); break; }

                // derive filename from timestamp
                char filepath[1024];
                snprintf(filepath, sizeof(filepath), "%s/key_%lu.pem", dirpath, (unsigned long)time(NULL));
                FILE *kf = fopen(filepath, "wb");
                if (!kf) { EC_KEY_free(eckey); fprintf(stderr, "[!] Error: no se pudo crear archivo de clave.\n"); break; }

                if (PEM_write_ECPrivateKey(kf, eckey, NULL, NULL, 0, NULL, NULL) != 1) {
                    fclose(kf); EC_KEY_free(eckey); fprintf(stderr, "[!] Error: fallo al escribir PEM.\n"); break;
                }
                fclose(kf);
                chmod(filepath, S_IRUSR | S_IWUSR);

                // extract private key as hex
                const BIGNUM *priv_bn = EC_KEY_get0_private_key(eckey);
                unsigned char privbuf[64];
                int priv_len = BN_num_bytes(priv_bn);
                memset(privbuf, 0, sizeof(privbuf));
                BN_bn2binpad(priv_bn, privbuf, 32);
                char hexpriv[65];
                for (int i = 0; i < 32; i++) sprintf(hexpriv + i*2, "%02x", privbuf[i]);
                hexpriv[64] = '\0';

                printf("[+] Clave generada: %s\n", filepath);
                printf("[+] Clave privada (hex): %s\n", hexpriv);

                EC_KEY_free(eckey);
                break;
            }
            case 'k':
            {
                strncpy(privkey_hex, optarg, sizeof(privkey_hex) - 1);
                break;
            }
            case 's':
            {
                if (chain->length == 0 || !chain->blocks[0]) {
                    printf("[!] No hay bloques disponibles en la cadena local.\n");
                    break;
                }
                printf(" - Bloque [0] | Hash: %s\n", chain->blocks[0]->hash);
                printf(" - Total de bloques: %zu\n", chain->length);
                break;
            }

            case 'S':
            {
                print_blockchain_json(chain);
                break;
            }

            case 't':
            {
                char from[32], to[32];
                double amount = 0.0;

                if(sscanf(optarg, "%31[^:]:%31[^:]:%lf", from, to, &amount) != 3)
                {
                    fprintf(stderr,"[!] Formato de transaccion invalido, Use. FROM:TO:AMOUNT\n");
                    break;
                }

                mempool_load(&mp, MEMPOOL_FILE);
                Transaction tx;
                memset(&tx, 0, sizeof(Transaction));

                uint64_t amt = (uint64_t)(amount * 1000000.0 + 0.5);
                uint64_t fee = 0;
                uint64_t nonce = (uint64_t)time(NULL);
                transaction_create(&tx, from, to, amt, fee, nonce, NULL);

                if (strlen(privkey_hex) == 0) {
                    fprintf(stderr, "[!] Error: se requiere --key <hex_priv> para firmar transacciones ECDSA.\n");
                    break;
                }

                if (transaction_sign(&tx, privkey_hex) != 0) {
                    fprintf(stderr, "[!] Error: fallo al firmar la transaccion con la clave proporcionada.\n");
                    break;
                }

                if (!state_validate_tx(chain, &mp, &tx))
                {
                    double current_bal = state_get_effective_balance(chain, &mp, from);
                    fprintf(stderr, "[RECHAZADA] Saldo insuficiente en la cuenta '%s'. Saldo disponible: %2.f\n", from, current_bal);
                    break;
                }

                if(mempool_add_tx(&mp, &tx) == 0)
                {
                    mempool_save(&mp, MEMPOOL_FILE);
                    printf("[+] Transaccion agregada a %s (%zu pend. en total)\n", MEMPOOL_FILE, mp.count);
                }
                else
                {
                    fprintf(stderr,"[!] Error al agregar transaccion a la Mempool.\n");
                }

                break;
            }

            case 'c':
            {
                char ip[64] = {0};
                int port = 0;
                if(sscanf(optarg, "%63[^:]:%d", ip, &port) != 2)
                {
                    fprintf(stderr,"[!] Formato IP/Puerto invalido. Use IP:PUERTO\n");
                    break;
                }
                int peer_fd = connect_to_peer(ip, (uint16_t)port);
                if(peer_fd >=0)
                {
                    peer_pool_add(&g_peer_pool, peer_fd, ip, (uint16_t)port);
                    MsgVersion v = {1, (uint64_t)time(NULL), (uint32_t)chain->length};
                    send_message(peer_fd, MSG_VERSION, &v, sizeof(MsgVersion));
                    printf("[+] Conectado al peer %s:%d\n", ip, port);
                }
                else
                {
                    fprintf(stderr,"[!] Fallo la conexion al peer %s:%d\n", ip, port);
                }

                break;
            }

            case 'b':
            {
                const char *addr = optarg;
                mempool_load(&mp, MEMPOOL_FILE);
                double confirmed = state_get_balance(chain, addr);
                double effecive = state_get_effective_balance(chain, &mp, addr);

                printf("account_state_address:%s\n",addr);
                printf("saldo_confirmado:%2f\n",confirmed);
                printf("saldo_efectivo:%2f\n",effecive);
                break;
            }

            case 'B':
            {
                const char *addr = optarg;
                mempool_load(&mp, MEMPOOL_FILE);
                double confirmed = state_get_balance(chain, addr);
                double effective = state_get_effective_balance(chain, &mp, addr);
                printf("{\"address\":\"%s\",\"confirmed\":%.6f,\"effective\":%.6f}\n",
                       addr, confirmed, effective);
                break;
            }

            case 'M':
            {
                mempool_load(&mp, MEMPOOL_FILE);
                print_mempool_json(&mp);
                break;
            }

            case 'p':
            {
                const char *validator_addr = optarg;
                mempool_load(&mp, MEMPOOL_FILE);

                if(mp.count == 0)
                {
                    printf("[!] No hay transacciones en la mempool para proponer un bloque. \n");
                    break;
                }

                const uint8_t *prev_hash = (const uint8_t *)chain->blocks[chain->length - 1]->hash;
                uint32_t tx_count = (mp.count > MAX_TX_PER_BLOCK) ? MAX_TX_PER_BLOCK : (uint32_t)mp.count;
                Block *new_block = block_create(prev_hash, mp.transactions, tx_count);

                if(!new_block)
                {
                    fprintf(stderr, "[!] Error al instanciar el bloque. \n");
                    break;
                }

                strncpy(new_block->proposer, validator_addr, sizeof(new_block->proposer) - 1);
                new_block->commit_signatures_count = 1;
                strncpy(new_block->commit_signatures[0], validator_addr, sizeof(new_block->commit_signatures[0]) - 1);
                block_calculate_hash(new_block);

                if (consensus_propose_block(chain, new_block, validator_addr))
                {
                    if(blockchain_add_block(chain, new_block, &val_set))
                    {
                        storage_save_blockchain(chain, CHAIN_FILE);

                        for(uint32_t i = 0; i < tx_count; i++)
                        {
                            mempool_remove_tx(&mp, &mp.transactions[0]);
                        }

                        mempool_save(&mp, MEMPOOL_FILE);
                        printf("[+] Bloque propuesto y agregado correctamente por %s\n", validator_addr);
                    }
                    else
                    {
                        fprintf(stderr, "[!] Error: El bloque propuesto no superó las validaciones del estado.\n");
                        block_free(new_block);
                    }
                }
                else
                {
                    fprintf(stderr, "[!] Rechazado: El nodo '%s' no tiene permisos de validador \n", validator_addr);
                    block_free(new_block);
                }
                break;
            }

            case 'h':
            {
                print_help(argv[0]);
                break;
            }

            default:
            {
                printf("Opción no válida.\n");
                break;
            }
        }
    }

    mempool_clear(&mp);
    blockchain_free(chain);
    return 0;
}
