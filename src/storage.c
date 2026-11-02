#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../lib/storage.h"

Blockchain *storage_load_blockchain(const char *filepath)
{
    if (!filepath) return NULL;

    FILE *f = fopen(filepath, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fclose(f);

    if (length <= 0) {
        return NULL;
    }

    return NULL;
}

int storage_save_blockchain(const Blockchain *chain, const char *filepath)
{
    if (!chain || !filepath) return -1;

    char tmpname[1024];
    snprintf(tmpname, sizeof(tmpname), "%s.tmp", filepath);

    FILE *f = fopen(tmpname, "wb");
    if (!f) return -1;

    fprintf(f, "[\n");
    for (size_t i = 0; i < chain->length; i++) {
        Block *b = chain->blocks[i];
        if (!b) continue;

        fprintf(f, "  {\n");
        fprintf(f, "    \"index\": %u,\n", b->index);
        fprintf(f, "    \"timestamp\": %ld,\n", (long)b->timestamp);
        fprintf(f, "    \"round\": %u,\n", b->round);
        fprintf(f, "    \"proposer\": \"%s\",\n", b->proposer);
        fprintf(f, "    \"prev_hash\": \"%s\",\n", b->prev_hash);
        fprintf(f, "    \"hash\": \"%s\",\n", b->hash);
        fprintf(f, "    \"signatures_count\": %u,\n", b->commit_signatures_count);
        fprintf(f, "    \"tx_count\": %u,\n", b->tx_count);

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

    fflush(f);
    if (fsync(fileno(f)) != 0) {
        fclose(f);
        unlink(tmpname);
        return -1;
    }
    fclose(f);

    chmod(tmpname, S_IRUSR | S_IWUSR);

    if (rename(tmpname, filepath) != 0) {
        unlink(tmpname);
        return -1;
    }

    return 0;
}
