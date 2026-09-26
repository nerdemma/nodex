#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../lib/storage.h"


Blockchain* storage_load_blockchain(const char *filepath)
{
FILE *f = fopen(filepath, "r");
    if(!f) return NULL;

fseek(f, 0, SEEK_END);
long length = ftell(f); 
fseek(f, 0, SEEK_SET);
char *buffer = malloc(length + 1);
fread(buffer, 1, length, f);
flose(f);
buffer[length] = '\0';

free(buffer);
return chain;
}




int storage_save_blockchain(const Blockchain *chain, const char *filepath)
{
    if (!chain || !filepath) return -1;

    char tmpname[1024];
    snprintf(tmpname, sizeof(tmpname), "%s.tmp", filepath);

    FILE *f = fopen(tmpname, "wb");
    if (!f) return -1;

    // Begin JSON array
    fprintf(f, "[\n");
    for (size_t i = 0; i < chain->length; i++) {
        Block *b = chain->blocks[i];
        fprintf(f, "  {\n");
        fprintf(f, "    \"index\": %u,\n", b->index);
        fprintf(f, "    \"timestamp\": %ld,\n", (long)b->timestamp);
        fprintf(f, "    \"round\": %u,\n", b->round);
        fprintf(f, "    \"proposer\": \"%s\",\n", b->proposer);
        fprintf(f, "    \"prev_hash\": \"%s\",\n", b->prev_hash);
        fprintf(f, "    \"hash\": \"%s\",\n", b->hash);
        fprintf(f, "    \"signatures_count\": %u,\n", b->commit_signatures_count);
        fprintf(f, "    \"tx_count\": %u,\n", b->tx_count);

        // Transactions array
        fprintf(f, "    \"transactions\": [\n");
        for (uint32_t j = 0; j < b->tx_count; j++) {
            Transaction *tx = &b->transactions[j];
            fprintf(f, "      {\n");
            fprintf(f, "        \"sender\": \"%s\",\n", tx->sender);
            fprintf(f, "        \"receiver\": \"%s\",\n", tx->receiver);
            fprintf(f, "        \"amount\": %.6f,\n", (double)tx->amount / 1000000.0);
            fprintf(f, "        \"fee\": %.6f,\n", (double)tx->fee / 1000000.0);
            fprintf(f, "        \"nonce\": %lu,\n", (unsigned long)tx->nonce);
            fprintf(f, "        \"timestamp\": %lu,\n", (unsigned long)tx->timestamp);
            fprintf(f, "        \"pubkey\": \"%s\",\n", tx->pubkey);
            fprintf(f, "        \"signature\": \"%s\"\n", tx->signature);
            fprintf(f, "      }%s\n", (j + 1 < b->tx_count) ? "," : "");
        }
        fprintf(f, "    ]\n");

        fprintf(f, "  }%s\n", (i + 1 < chain->length) ? "," : "");
    }
    fprintf(f, "]\n");

    // flush and sync
    fflush(f);
    if (fsync(fileno(f)) != 0) {
        fclose(f);
        unlink(tmpname);
        return -1;
    }
    fclose(f);

    // set restrictive permissions
    chmod(tmpname, S_IRUSR | S_IWUSR);

    // atomic rename
    if (rename(tmpname, filepath) != 0) {
        unlink(tmpname);
        return -1;
    }

    return 0;
}
