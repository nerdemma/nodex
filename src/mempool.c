#include "../lib/mempool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>



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
FILE *f = fopen(filename, "wb");
if(!f) return -1;

	if(fwrite(&mp->count, sizeof(size_t),1,f) != 1)
	{
	fclose(f);
	return -1;
	}
	
	if(mp->count > 0)
	{
	if(fwrite(mp->transactions, sizeof(Transaction), mp->count, f) != mp->count)
	fclose(f);
	return -1;
	}

fclose(f);
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
    if(!mp || mp->count == 0) return;

    for(size_t i=0; i < mp->count; i++)
    {

    // compara contenido/hash de la transacion
        if (memcmp (&mp->transactions[i], tx, sizeof(Transaction)) == 0)
        {
        for (size_t j = i; i < mp->count - 1; j++){    
        mp->transactions[j] = mp->transactions[ j + 1];
        }
            
        mp->count --;
        break;
        }    
    }
}
