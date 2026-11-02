CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lcrypto

OBJS = src/blockchain.o src/block.o src/transaction.o \
       src/mempool.o src/utils.o src/net.o src/discovery.o src/peer_pool.o \
       src/p2p_sync.o src/state.o src/consensus.o src/storage.o \
       src/merkle.o
       
all: node_daemon node_cli


node_daemon: src/node_daemon.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

node_cli: src/node_cli.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f src/*.o node_daemon node_cli

