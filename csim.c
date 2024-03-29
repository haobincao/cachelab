#include <getopt.h>  // getopt, optarg
#include <stdlib.h>  // exit, atoi, malloc, free
#include <stdio.h>   // printf, fprintf, stderr, fopen, fclose, FILE
#include <limits.h>  // ULONG_MAX
#include <string.h>  // strcmp, strerror
#include <errno.h>   // errno

/* fast base-2 integer logarithm */
#define INT_LOG2(x) (31 - __builtin_clz(x))
#define NOT_POWER2(x) (__builtin_clz(x) + __builtin_ctz(x) != 31)

/* tag_bits = ADDRESS_LENGTH - set_bits - block_bits */
#define ADDRESS_LENGTH 64

/**
 * Print program usage (no need to modify).
 */
static void print_usage() {
    printf("Usage: csim [-hv] -S <num> -K <num> -B <num> -p <policy> -t <file>\n");
    printf("Options:\n");
    printf("  -h           Print this help message.\n");
    printf("  -v           Optional verbose flag.\n");
    printf("  -S <num>     Number of sets.           (must be > 0)\n");
    printf("  -K <num>     Number of lines per set.  (must be > 0)\n");
    printf("  -B <num>     Number of bytes per line. (must be > 0)\n");
    printf("  -p <policy>  Eviction policy. (one of 'FIFO', 'LRU')\n");
    printf("  -t <file>    Trace file.\n\n");
    printf("Examples:\n");
    printf("$ ./csim    -S 16  -K 1 -B 16 -p LRU -t traces/yi2.trace\n");
    printf("$ ./csim -v -S 256 -K 2 -B 16 -p LRU -t traces/yi2.trace\n");
}

/* Parameters set by command-line args (no need to modify) */
int verbose = 0;   // print trace if 1
int S = 0;         // number of sets
int K = 0;         // lines per set
int B = 0;         // bytes per line

typedef enum { FIFO = 1, LRU = 2 } Policy;
Policy policy;     // 0 (undefined) by default

FILE *trace_fp = NULL;

/**
 * Parse input arguments and set verbose, S, K, B, policy, trace_fp.
 *
 * TODO: Finish implementation
 */
static void parse_arguments(int argc, char **argv) {
    char c;
    while ((c = getopt(argc, argv, "S:K:B:p:t:vh")) != -1) {
        switch(c) {
            case 'S':
                S = atoi(optarg);
                if (NOT_POWER2(S)) {
                    fprintf(stderr, "ERROR: S must be a power of 2\n");
                    exit(1);
                }
                break;
            case 'K':
                // TODO
                K = atoi(optarg);
                //fprintf(stderr, "ERROR: %s: %s\n", optarg, strerror(errno));
                break;
            case 'B':
                // TODO
                B = atoi(optarg);
                break;
            case 'p':
                if (!strcmp(optarg, "FIFO")) {
                    policy =  1;
                }
                // TODO: parse LRU, exit with error for unknown policy
                else if (!strcmp(optarg, "LRU")) {
                    policy = 2;
                }
                else {
                    fprintf(stderr, "ERROR: Unknown policy %s\n", optarg);
                    exit(1);
                }
                break;
            case 't':
                // TODO: open file trace_fp for reading
                trace_fp = fopen(optarg, "r");

                if (!trace_fp) {
                    fprintf(stderr, "ERROR: %s: %s\n", optarg, strerror(errno));
                    exit(1);
                }
                
                //fclose(optarg);
                break;
            case 'v':
                // TODO
                verbose = 1;
                break;
            case 'h':
                // TODO
                print_usage();
                exit(0);
            default:
                print_usage();
                exit(1);
        }
    }

    /* Make sure that all required command line args were specified and valid */
    if (S <= 0 || K <= 0 || B <= 0 || policy == 0 || !trace_fp) {
        printf("ERROR: Negative or missing command line arguments\n");
        print_usage();
        if (trace_fp) {
            fclose(trace_fp);
        }
        exit(1);
    }

    /* Other setup if needed */



}

/**
 * Cache data structures
 * TODO: Define your own!
 */
struct Line {
    char valid;
    unsigned long tag;
    unsigned int LRU_counter;
    unsigned int FIFO_counter;
};

struct Line** cache = NULL;


/**
 * Allocate cache data structures.
 *
 * This function dynamically allocates (with malloc) data structures for each of
 * the `S` sets and `K` lines per set.
 *
 * TODO: Implement
 */
static void allocate_cache() {
    cache = (struct Line **)malloc(S * sizeof(struct Line*));
    for (int i = 0; i < S; i++) {
        cache[i] = (struct Line*) malloc(K * sizeof(struct Line));
    } 


}

/**
 * Deallocate cache data structures.
 *
 * This function deallocates (with free) the cache data structures of each
 * set and line.
 *
 * TODO: Implement
 */
static void free_cache() {
    for (int i = 0; i < S; i++) {
        free(cache[i]);
    }
    free(cache);

}

/* Counters used to record cache statistics */
int miss_count     = 0;
int hit_count      = 0;
int eviction_count = 0;

static void addLRU(unsigned long set, int index) {
    for (int i = 0; i < K; i++) {
        if (i != index) {
            cache[set][i].LRU_counter++;
        }
    }
}

/**
 * Simulate a memory access.
 *
 * If the line is already in the cache, increase `hit_count`; otherwise,
 * increase `miss_count`; increase `eviction_count` if another line must be
 * evicted. This function also updates the metadata used to implement eviction
 * policies (LRU, FIFO).
 *
 * TODO: Implement
 */
static void access_data(unsigned long addr) {
    unsigned long tag = addr >> (INT_LOG2(S) + INT_LOG2(B));
    unsigned long temp = 0xFFFFFFFFFFFFFFFF >> (64 - INT_LOG2(S));
    unsigned long set = (addr >> INT_LOG2(B) ) & (temp);
    if (INT_LOG2(S) == 0) {
        set = 0;
    } 
    int cnt = 0;
    int cnt_full = 0;
    int space_index = 0;
    unsigned int curr = 0;
    for (int i = 0; i < K; i++) {
        if (cache[set][i].tag == tag && cache[set][i].valid == 1) {
            hit_count++;
            //printf("%s", "hit ");
            cache[set][i].LRU_counter = 1;
            addLRU(set,i);
            break;
        }
        else {
            if (cache[set][i].valid == 1) {
                cnt_full++;
                if (cache[set][i].FIFO_counter > curr) {
                    curr = cache[set][i].FIFO_counter;
                }
            }
            else {
                if (space_index == 0) {
                    space_index = i;
                }
            }
            cnt++;
        }
    }
    if (cnt == K) {
        if (cnt_full == K) {
            miss_count++;
            eviction_count++;
            //printf("%s", "miss eviction ");
            if (policy == 1) {
                unsigned int FIFO_curr = cache[set][0].FIFO_counter;
                space_index = 0;
                for (int i = 0; i < K; i++) {
                    if (cache[set][i].FIFO_counter < FIFO_curr) {
                        space_index = i;
                        FIFO_curr = cache[set][i].FIFO_counter;
                    }
                }
                cache[set][space_index].tag = tag;
                cache[set][space_index].valid = 1;
                cache[set][space_index].LRU_counter = 1;
                addLRU(set,space_index);
                cache[set][space_index].FIFO_counter = curr + 1;
            }
            else if (policy == 2) {
                unsigned int LRU_curr = cache[set][0].LRU_counter;
                space_index = 0;
                for (int i = 0; i < K; i++) {
                    if (cache[set][i].LRU_counter > LRU_curr) {
                        space_index = i;
                        LRU_curr = cache[set][i].LRU_counter;
                    }
                }
                cache[set][space_index].tag = tag;
                cache[set][space_index].valid = 1;
                cache[set][space_index].LRU_counter = 1;
                addLRU(set,space_index);
                cache[set][space_index].FIFO_counter = curr + 1;
            }
        }
        else {
            //printf("%s", "miss ");
            miss_count++;
            cache[set][space_index].tag = tag;
            cache[set][space_index].valid = 1;
            cache[set][space_index].LRU_counter = 1;
            addLRU(set,space_index);
            cache[set][space_index].FIFO_counter = curr + 1;
        }
    }
    //printf("Access to %016lx\n", addr);
}


/**
 * Replay the input trace.
 *
 * This function:
 * - reads lines (e.g., using fgets) from the file handle `trace_fp` (a global variable)
 * - skips lines not starting with ` S`, ` L` or ` M`
 * - parses the memory address (unsigned long, in hex) and len (unsigned int, in decimal)
 *   from each input line
 * - calls `access_data(address)` for each access to a cache line
 *
 * TODO: Implement
 */
static void replay_trace() {
    char buf[1024];
    unsigned long int addr;
    unsigned int len;
    int modify = 1;

    while (fgets(buf, sizeof(buf), trace_fp) != NULL) {
        if (buf[0] == 'I') continue;
        if (buf[1] == 'M') {
            modify = 2;
        }
        sscanf(buf + 3, "%lx,%u", &addr, &len);
        //for (int i = 0; i < modify; i++) {
        unsigned long int end_addr = addr + len;
        unsigned long int start_block = addr / B;
        unsigned long int end_block = (end_addr - 1) / B;

        for (unsigned long int block = start_block; block <= end_block; block++) {
            // Call access_data for each block within the address range
            unsigned long block_addr = block * B;
            for (int i = 0; i < modify; i++) {
            access_data(block_addr);
            }
            //printf("\n");
        }
        modify = 1;
        //}
    }
}

/**
 * Print cache statistics (DO NOT MODIFY).
 */
static void print_summary(int hits, int misses, int evictions) {
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
}

int main(int argc, char **argv) {
    parse_arguments(argc, argv);  // set global variables used by simulation
    allocate_cache();             // allocate data structures of cache
    replay_trace();               // simulate the trace and update counts
    free_cache();                 // deallocate data structures of cache
    fclose(trace_fp);             // close trace file
    print_summary(hit_count, miss_count, eviction_count);  // print counts
    return 0;
}
