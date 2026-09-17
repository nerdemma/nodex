#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "../lib/blockchain.h"
#include "../lib/mempool.h"
#include "../lib/block.h"
#include "../lib/peer_pool.h"
#include "../lib/net.h"
#include "../lib/state.h"
#include "../lib/consensus.h"
#include "../lib/storage.h"
#include "../lib/utils.h"

ValidatorSet val_set;


void print_help(const char *prog_name)
{
    printf("Uso: %s [OPCIONES]\n", prog_name);
    printf(" -s --status                Ver el estado e integridad de la cadena local\n");
    printf(" -t --tx<from:to:amount>    Crear una nueva trasaccion de pureba (ej: ALICE:BOB:25.5\n");
    printf(" -c --connect <ip:port>     Connectar a un peer remoto (ej:192.168.0.2:8333)\n");
    printf(" -b --balance <address>     Consultar el saldo de una cuenta \n");
    printf(" -b --propose <validator>   Proponer/validar un nuevo bloque de consenso \n");
    printf(" -h --help                  Mostrar esta ayuda.\n");

}

int main(int argc, char *argv[])
{

if(argc < 2) { print_help(argv[0]); return 0;}

peer_pool_init(&g_peer_pool);
Blockchain *chain = blockchain_init(CHAIN_FILE);
 
    if(!chain)
    {
    fprintf(stderr, "[!] Error: no se pudo cargar la blockchain desde el disco.\n");	
	return 1;
    }

Mempool mp;
mempool_init(&mp);

static struct option long_options[] = {
{"status",  no_argument,        0,  's'},
{"tx",      required_argument,  0,  't'},
{"connect", required_argument,  0,  'c'},
{"balance", required_argument,  0,  'b'},
{"propose", required_argument,  0,  'p'},
{"help",    no_argument,        0,  'h'},
{0, 0, 0, 0}
};

int opt;
int option_index = 0;


while ((opt = getopt_long(argc, argv, "smt:c:b:p:h", long_options, &option_index)) != -1)
    {
		switch (opt)
        {
        
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
        
        
		case 't':
		{
		char from[32], to[32];
		double amount = 0.0;
		
			if(sscanf(optarg, "%31[^:]:%31[^:]:%lf", from, to, &amount) != 3)
			{
			fprintf(stderr,"[!] Formato de transacion invalido, Use. FROM:TO:AMOUNT\n");
			break;	
			}
		
		mempool_load(&mp, MEMPOOL_FILE);
		Transaction tx;
		memset(&tx, 0, sizeof(Transaction));
		transaction_create(&tx, from, to, amount, 0.0);		
		
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
		
        case 'p':
        {
        const char *validator_addr = optarg;
        mempool_load(&mp, MEMPOOL_FILE);

        if(mp.count == 0)
        {
        printf("[!] No hay transacciones en la mempool para proponer un bloque. \n");
        break;
        }

        // Obtiene el hash del bloque previo
        const uint8_t *prev_hash = (const uint8_t *)chain->blocks[chain->length - 1]->hash;
        
        // crea el bloque inicializando con las transaciones de la mempool
        uint32_t tx_count = (mp.count > MAX_TX_PER_BLOCK) ? MAX_TX_PER_BLOCK : (uint32_t)mp.count;
        Block *new_block = block_create(prev_hash, mp.transactions, tx_count);        
        
        if(!new_block)
        {
        fprintf(stderr, "[!] Error al instanciar el bloque. \n");
        break;
        }

        // se valida la propuesta mediante el consenso
        if (consensus_propose_block(chain, new_block, validator_addr))
        {
            if(blockchain_add_block(chain, new_block, &val_set)) 
            {
           storage_save_blockchain(chain, CHAIN_FILE);

                // remueve las transaciones procesadas de la mempool
                for(uint32_t i = 0; i < tx_count; i++)
                {
                mempool_remove_tx(&mp, &mp.transactions[0]);
                }
            mempool_save(&mp, MEMPOOL_FILE);

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
