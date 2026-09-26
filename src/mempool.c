#include "../lib/mempool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>



void mempool_init(Mempool *mp) {
    if (!mp) return;
    memset(mp->transactions, 0, sizeof(mp->transactions)); mp->count=0;
}


int mempool_add_tx(Mempool *mp, const Transaction *tx)
{
    if (!mp || !tx || mp->count >= MAX_MEMPOOL_TXS) return -1;
    mp->transactions[mp->count] = *tx;
    mp->count++;
    return 0;
}



void mempool_clear(Mempool *mp)
{
	if(!mp) return;
	memset(mp->transactions, 0 , sizeof(mp->transactions));
    mp->count =0;
}


int mempool_save(const Mempool *mp, const char *filename)
{
	if(!mp || !filename) return -1;

	char tmpname[512];
	snprintf(tmpname, sizeof(tmpname), "%s.tmp", filename);

	FILE *f = fopen(tmpname, "wb");
	if(!f) return -1;

	if(fwrite(&mp->count, sizeof(size_t),1,f) != 1)
	{
		fclose(f);
		unlink(tmpname);
		return -1;
	}

	if(mp->count > 0)
	{
		if(fwrite(mp->transactions, sizeof(Transaction), mp->count, f) != mp->count)
		{
			fclose(f);
			unlink(tmpname);
			return -1;
		}
	}

	fflush(f);
	fsync(fileno(f));
	fclose(f);

	// set restrictive permissions
	chmod(tmpname, S_IRUSR | S_IWUSR);

	// atomic rename
	if(rename(tmpname, filename) != 0)
	{
		unlink(tmpname);
		return -1;
	}

	return 0;
}

int mempool_load(Mempool *mp, const char *filename)
{
	if(!mp||!filename) return -1;
mempool_clear(mp);
FILE *f = fopen(filename, "rb");

	if(!f) return 0;
size_t count = 0;

	if(fread(&count, sizeof(size_t),1,f) != 1)
	{
	fclose(f);
	return 0;
	}

	if (count > MAX_MEMPOOL_TXS)
	{
	fclose(f);
	return -1;	
	}
	
	if(count > 0)
	{
		if(fread(mp->transactions, sizeof(Transaction), count, f) != count)
		{
		fclose(f);
		return -1;
		}	
	}
	
	mp->count = count;
	fclose(f);
	return 0;
}



void mempool_remove_tx(Mempool *mp, const Transaction *tx)
{
    if(!mp || mp->count == 0 || !tx) return;

    for(size_t i = 0; i < mp->count; i++)
    {
        if (memcmp(&mp->transactions[i], tx, sizeof(Transaction)) == 0)
        {
            for (size_t j = i; j < mp->count - 1; j++)
            {
                mp->transactions[j] = mp->transactions[j + 1];
            }

            memset(&mp->transactions[mp->count - 1], 0, sizeof(Transaction));
            mp->count--;
            break;
        }
    }
}
