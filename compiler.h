#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t u32;

#define LEN(X) (sizeof((X)) / sizeof(*(X)))

// #define log(...) fprintf(stderr, __VA_ARGS__)
#define log(...) ;

/** input = input as bytes, mem = main memory, s = length(mem) */
bool *filled;
u32 *input, *mem, s;

void finish(int status);

#define SADGE(s)                                                               \
  {                                                                            \
    printf(s "\n");                                                            \
    finish(1);                                                                 \
  }

FILE *cacheFile;

typedef struct TsfavCacheData {
  u32 searchIdx;
  u32 resolvedIdx;
  u32 value;
  u32 A;
  u32 B;
  u32 C;
  u32 D;
} TsfavCacheData;

typedef struct TsfavCacheNode {
  TsfavCacheData data;
  struct TsfavCacheNode *next;
} TsfavCacheNode;

TsfavCacheNode *tsfavHead;
TsfavCacheNode *tsfavTail;

#define CACHE_FILE "cache/tsfav"

#define SETSZ (1 << 18)
#define MINLOOKUP (1 << 11)
// value in set if thing[value % n % SETSZ] == value
typedef struct Set {
  u32 n;
  u32 thing[SETSZ];
} Set;

FILE *labelsFile;

typedef struct Label {
  u32 pos;
  char *name;
  bool is_from_label_pass;
} Label;
Label *labels;
u32 num_labels = 0;

#define LABEL_NOT_DEFINED (-1)

typedef struct Mnemonic {
  u32 value;
  char *name;
} Mnemonic;

#define NO_NEXT_POS (-1)

FILE *inputFile;
