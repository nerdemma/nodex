#include "../lib/utils.h"
#include <stdio.h>
#include <openssl/evp.h>
#include "../lib/blockchain.h"


void print_blockchain_json(const Blockchain *chain)
{
if(!chain) return;
printf("{\"status\":\"success\",\"data\":[");
for(size_t i=0; i < chain->length; i++)
    {
    Block *b = chain->blocks[i];
    printf("{\"index\":%u,\"hash\":\"%s\",\"prev_hash\":\"%s\",\"tx_count\":%u}%s",
    
        b->index,
        b->hash,
        b->prev_hash,
        b->tx_count,
        (i < chain->length - 1) ? "," : "");

    printf("]}\n");
}
}

void sha256(const uint8_t *data, size_t len, uint8_t out[32])
{
EVP_MD_CTX *ctx = EVP_MD_CTX_new();
EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
EVP_DigestUpdate(ctx, data, len);
unsigned int out_len;
EVP_DigestFinal_ex(ctx, out, &out_len);
EVP_MD_CTX_free(ctx);
}

void double_sha256(const uint8_t *data, size_t len, uint8_t out[32])
{
uint8_t first[32];
sha256(data, len, first);
sha256(first, 32, out);
}

void hex_encode(const uint8_t *in, size_t len, char *out) {
for (size_t i = 0; i < len; i++) {
sprintf(out + (i * 2), "%02x", in[i]);
}
out[len * 2] = ' ';
}


void print_hash(const uint8_t *hash, size_t len)
{
if(!hash) return;
    for(size_t i = 0; i < len; i++)
    {
    printf("%02x", hash[i]);
    }
}



void blockchain_print(const Blockchain *chain, const ValidatorSet *val_set)
{
if(!chain) return;

printf("[+] Estado de la cadena: Total Bloques = %zu | Valida = %s\n",
           chain->length, blockchain_is_valid(chain, val_set) ? "SI" : "NO");
           
    for(size_t i = 0; i < chain->length; i++)
    {
    printf(" - Bloque [%zu] OK | Hash:", i);
    print_hash((const uint8_t *) chain->blocks[i]->hash, 32);
    printf("\n");
    }

}
