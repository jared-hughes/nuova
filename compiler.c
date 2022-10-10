#include "./compiler.h"

u32 P(u32 x) {
  x ^= x >> 17;
  x *= 0xed5ad4bbU;
  x ^= x >> 11;
  x *= 0xac4c1b51U;
  x ^= x >> 15;
  x *= 0x31848babU;
  x ^= x >> 14;
  return x;
}

u32 inverseP(u32 x) {
  x ^= x >> 14 ^ x >> 28;
  x *= 0x32b21703U;
  x ^= x >> 15 ^ x >> 30;
  x *= 0x469e0db1U;
  x ^= x >> 11 ^ x >> 22;
  x *= 0x79a85073U;
  x ^= x >> 17;
  return x;
}

void finish(int status) {
  free(filled);
  free(input);
  free(mem);
  free(labels);
  for (; tsfavHead != NULL;) {
    TsfavCacheNode *t = tsfavHead;
    tsfavHead = tsfavHead->next;
    free(t);
  }
  fclose(labelsFile);
  fclose(cacheFile);
  fclose(inputFile);
  exit(status);
}

/** gw: ensure mem is at least length a+1, filling in mem with 0s */
void gw(u32 a) {
  if (__builtin_expect(a >= s, 0)) {
    u32 i = s;
    s = a + 1;
    if (a > 0x100000)
      SADGE("Too Long");
    mem = realloc(mem, s * sizeof(*mem));
    input = realloc(input, s * sizeof(*input));
    filled = realloc(filled, s * sizeof(*filled));
    for (; i < s; i++) {
      mem[i] = 0;
      input[i] = 0;
      filled[i] = 0;
    }
  }
}

/** mem set: set mem[idx] <- v at initial load, ensuring length at least idx+1*/
void ms_simple_inner(u32 idx, u32 v) {
  gw(idx);
  if (filled[idx]) {
    log("Already filled, at %08X\n", idx);
    finish(1);
  }
  u32 xor = v ^ P(idx);
  if (xor >> 8) {
    if (idx == 0) {
      SADGE("Can't 4-byter at beginning")
    } else if (filled[idx - 1]) {
      if ((input[idx - 1] >> 8 > 0) || (input[idx - 1] & 0xF) != 0xF)
        SADGE("Not F, can't fill x4")
    } else {
      filled[idx - 1] = 1;
      u32 p = P(idx - 1);
      if (p >> 8 == 0 && idx > 1)
        SADGE("Collision possible")
      mem[idx - 1] = p ^ 0x0F;
      input[idx - 1] = 0xFF;
    }
  }
  filled[idx] = 1;
  mem[idx] = v;
  input[idx] = xor;
}

bool get_filled(u32 idx) {
  gw(idx);
  return filled[idx];
}

u32 padding_char(u32 idx) {
  for (u32 c = 0; c < 256; c++) {
    if ((c & 0xF) == 0xF)
      continue;
    u32 res = P(idx) ^ c;
    if (!(res >> 8 == 0 && ((res & 0xF) == 0xF || (res & 0xF) == 0))) {
      return c;
    }
  }
}

void ms_simple(u32 idx, u32 v) {
  verbose_printf("ms_simple(0x%08X, 0x%08X)\n", idx, v);
  msSimpleCount++;
  ms_simple_inner(idx, v);
}

bool can_ms_simple(u32 idx, u32 v) {
  return idx == 0 || !get_filled(idx - 1) || (v ^ P(idx)) >> 8 == 0 ||
         ((input[idx - 1] >> 8 == 0) && (input[idx - 1] & 0xF == 0xF));
}

/**
 * Fills in [start, ..., end] with all padding: PPP every time, exactly
 * end - start + 1 PPPs (no instr )
 * Start and end are inclusive
 */
void padding(u32 start, u32 end) {
  for (u32 j = start; j <= end; j++) {
    // TODO: avoid accidental instruction
    ms_simple_inner(j, P(j) ^ padding_char(j)); // bunch of PPP;
  }
}

void emit_from_mem() {
  printf("Emitting!\n");
  FILE *out = fopen("bin/prog", "wb");
  u32 prev_f = 0;
  u32 bytes = 0;
  for (u32 i = 0; i < s; i++) {
    u32 x = get_filled(i) ? input[i] : padding_char(i);
    if (prev_f) {
      prev_f = 0;
      fputc(x >> 24, out);
      fputc(x >> 16, out);
      fputc(x >> 8, out);
      fputc(x, out);
      bytes += 4;
    } else {
      fputc(x, out);
      bytes++;
      if ((x & 0xF) == 0xF)
        prev_f = 1;
    }
  }
  fclose(out);
  printf("Bytes: %d\n", bytes);
  printf("initMemset count: %d\n", initMemsetCount);
  printf("initMemsetZero count: %d\n", initMemsetZeroCount);
  printf("msSimple count: %d\n", msSimpleCount);
}

void printData(TsfavCacheData data) {
  log("{0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X}",
      data.searchIdx, data.resolvedIdx, data.value, data.A, data.B, data.C,
      data.D);
}

TsfavCacheData *find_in_cache(u32 searchIdx, u32 value) {
  TsfavCacheNode *node = tsfavHead;
  while (node != NULL) {
    if (node->data.searchIdx == searchIdx && node->data.value == value) {
      log("Found in cache: ");
      printData(node->data);
      log("\n");
      return &(node->data);
    }
    node = node->next;
  }
  return NULL;
}

void add_to_mem_cache(TsfavCacheData data) {
  // log("Adding to cache: ");
  // printData(data);
  // log("\n");
  TsfavCacheNode *node = malloc(sizeof(TsfavCacheNode));
  node->data = data;
  node->next = NULL;
  if (tsfavHead == NULL) {
    tsfavHead = node;
    tsfavTail = node;
  } else {
    tsfavTail->next = node;
    tsfavTail = node;
  }
}

void add_to_cache(TsfavCacheData data) {
  add_to_mem_cache(data);
  if (cacheFile != NULL) {
    // log("Writing to file\n");
    fwrite(&data, sizeof(TsfavCacheData), 1, cacheFile);
    fflush(cacheFile);
  } else
    log("Cache file is null, so not appending to disk\n");
}

void build_cache() {
  TsfavCacheData data;
  cacheFile = fopen(CACHE_FILE, "rb");
  if (cacheFile) {
    while (1) {
      size_t count = fread(&data, sizeof(TsfavCacheData), 1, cacheFile);
      if (count != 1) {
        if (feof(cacheFile))
          break;
        else
          SADGE("Cache file reading oopsie")
      }
      add_to_cache(data);
    }
    fclose(cacheFile);
  }
  cacheFile = fopen(CACHE_FILE, "ab");
}

bool rangeIsOpen(u32 start, u32 end) {
  for (u32 i = start; i <= end; i++) {
    if (get_filled(i))
      return false;
  }
  return true;
}

/**
 * Set the value of `a` to `value` at idx + 21.
 * Requires writing spawn mem from idx to idx + 21 inclusive
 * Tramples over b and c but small
 **/
TsfavCacheData try_set_force_a_value(u32 searchIdx, u32 idx, u32 searchValue,
                                     u32 value) {
  if (!rangeIsOpen(idx, idx + 21))
    return (TsfavCacheData){0, 0, 0, 0, 0, 0, 0};
  for (u32 A = 0; A < 256; A++) {
    if ((A & 0xF) == 0xF)
      continue;
    for (u32 B = 0; B < 256; B++) {
      if ((B & 0xF) == 0xF)
        continue;
      for (u32 C = 0; C < 256; C++) {
        if ((C & 0xF) == 0xF)
          continue;
        for (u32 D = 0; D < 256; D++) {
          if ((D & 0xF) == 0xF)
            continue;
          u32 a, b, c, d;
          b = A ^ P(idx + 2);
          b = P(b);
          c = B ^ P(idx + 5);
          b = P(b), c = P(c);
          a = b + c;
          a = P(a), b = P(b), c = P(c);
          b = a;
          a = P(a), b = P(b), c = P(c);
          c = C ^ P(idx + 12);
          a = P(a), b = P(b), c = P(c);
          a = b + c;
          a = P(a), b = P(b), c = P(c);
          b = a;
          a = P(a), b = P(b), c = P(c);
          c = D ^ P(idx + 19);
          a = P(a), b = P(b), c = P(c);
          a = b + c;
          if (a == inverseP(value)) {
            return (TsfavCacheData){searchIdx, idx, searchValue, A, B, C, D};
          }
        }
      }
    }
  }
  return (TsfavCacheData){0, 0, 0, 0, 0, 0, 0};
}

void force_a_value_from_struct(TsfavCacheData data) {
  u32 idx = data.resolvedIdx;
  // add_to_cache();
  ms_simple_inner(idx + 1, 0x1F);                  // b =
  ms_simple_inner(idx + 2, data.A ^ P(idx + 2));   // A ^ P(idx + 2);
  ms_simple_inner(idx + 4, 0x2F);                  // P(); c =
  ms_simple_inner(idx + 5, data.B ^ P(idx + 5));   // B ^ P(idx + 5);
  ms_simple_inner(idx + 7, 0xB0);                  // P(); a = b + c;
  ms_simple_inner(idx + 9, 0x60);                  // P(); b = a;
  ms_simple_inner(idx + 11, 0x2F);                 // P(); c =
  ms_simple_inner(idx + 12, data.C ^ P(idx + 12)); // C ^ P(idx + 12);
  ms_simple_inner(idx + 14, 0xB0);                 // P(); a = b + c;
  ms_simple_inner(idx + 16, 0x60);                 // P(); b = a;
  ms_simple_inner(idx + 18, 0x2F);                 // P(); c =
  ms_simple_inner(idx + 19, data.D ^ P(idx + 19)); // D ^ P(idx + 19);
  ms_simple_inner(idx + 21, 0xB0);                 // P(); a = b + c = value
}

/**
 * idx: a = value
 * Requires at least 21 mem before that (so 22 total)
 * Might need to backtrack some, so leave some space
 */
void force_a_value_inner(u32 idx, u32 value) {
  u32 searchIdx = idx;
  u32 searchValue = value;
  TsfavCacheData *data = find_in_cache(idx, value);
  if (data != NULL) {
    force_a_value_from_struct(*data);
    return;
  }
  log("cache miss 0x%08X 0x%08X\n", idx, value);
  for (;; idx--) {
    if (get_filled(idx)) {
      printf("No valid position in force_a at idx %08X\n", idx);
      finish(2);
    }
    log("Trying idx 0x%08X\n", idx);
    TsfavCacheData res =
        try_set_force_a_value(searchIdx, idx - 21, searchValue, value);
    if (res.searchIdx > 0) {
      force_a_value_from_struct(res);
      add_to_cache(res);
      return;
    }
    value = inverseP(value);
  }
}

void force_a_value(u32 idx, u32 value) {
  printf("0x%08X: .forceA 0x%08X\n", idx, value);
  force_a_value_inner(idx, value);
}

/**
 * Putchars value&255 sometime after idx. Returns the new last_pos
 */
u32 force_putchar(u32 idx, u32 value) {
  for (;; idx++) {
    if (!can_ms_simple(idx, 0x0F)) {
      continue;
    }
    for (u32 X = 0; X <= 0xFF; X++) {
      if (idx > 0 && (X & 15) == 15)
        continue;
      if ((P(X ^ P(idx + 1)) & 0xFF) == (value & 0xFF)) {
        ms_simple_inner(idx, 0x0F);               // PPP; a =
        ms_simple_inner(idx + 1, X ^ P(idx + 1)); // X ^ P(idx + 1)
        ms_simple_inner(idx + 3, 0xD0);           // PPP; putchar(a)
        return idx + 3;
      }
    }
  }
}

// SETSZ (1 << 18)
// MINLOOKUP (1 << 11)
// key = v % n % SETSZ
u32 MODS[] = {
    0,      268556, 266594, 262998, 262200, 266760, 263300, 264101, 264945,
    264688, 263644, 268799, 262339, 264458, 262869, 263532, 266197, 262400,
    265597, 266914, 263816, 267806, 263218, 265844, 265178, 266528, 265728,
    262553, 264788, 263580, 263293, 272927, 267147, 275720, 264410, 266000,
    273028, 262513, 265460, 265939, 266048, 265051, 262396, 264565, 265046,
    262921, 264533, 263678, 262828, 266683, 267995, 264316, 264954, 264960,
    264658, 264697, 265029, 267166, 262788, 272741, 264322, 266193, 263026,
    263604, 263296, 263843, 262636, 268127, 265140, 265273, 262226, 269344,
    263917, 262713, 263194, 263395, 277860, 264056, 266434, 266083, 265876,
    265298, 262301, 262803, 270084, 265861, 262748, 264343, 272010, 262588,
    265414, 262810, 265548, 265578, 262692, 262408, 262960, 264344, 263881,
    262933, 262338, 266613, 264878, 275967, 264353, 267757, 262674, 262700,
    263829, 264030, 263027, 273019, 262911, 263621, 263223, 264711, 262150,
    262809, 262178, 271027, 270432, 267378, 264596, 267265, 263866, 262285,
    263637, 278314, 272662, 265719, 264757, 264130, 262309, 262386, 263981,
    264732, 266344, 264994, 266785, 274996, 262769, 265548, 265022, 262883,
    265206, 262490, 264260, 264589, 262954, 263562, 263213, 262177, 263000,
    264884, 262898, 275057, 270493, 262253, 263626, 264866, 264843, 262505,
    265494, 264475, 264400, 277913, 263956, 266528, 265688, 270545, 262218,
    263069, 269278, 264588, 270709, 262201, 263333, 264365, 262782, 263007,
    266473, 266411, 262220, 263860, 263232, 272093, 263555, 265758, 266579,
    263136, 275772, 262914, 265626, 262949, 266161, 262506, 273037, 262807,
    263028, 268448, 262445, 266752, 264885, 269833, 264363, 264974, 262866,
    266536, 264655, 265213, 266664, 262921, 264158, 266078, 272643, 263639,
    268604, 264030, 270489, 264731, 262541, 274526, 263337, 270678, 263736,
    262873, 265167, 263683, 263697, 271390, 262482, 263926, 264695, 270607,
    262389, 262744, 264290, 263460, 263453, 263578, 263009, 265173, 267117,
    267579, 262442, 264243, 262860, 263114, 266765, 262302, 263944, 263505,
    265170, 264120, 262485, 262962,
};

u32 find_hash_mod(u32 value) {
  u32 round[SETSZ] = {0};
  for (u32 n = SETSZ;; n++) {
    u32 v = value;
    for (u32 i = 0;; i++, v = inverseP(v)) {
      u32 key = v % n % SETSZ;
      if (round[key] == n) {
        // collision!
        if (i > MINLOOKUP) {
          // big enough
          return n;
        } else {
          // try the next n
          break;
        }
      }
      round[key] = n;
    }
  }
}

void find_hash_mods() {
  printf("u32 MODS[] = {0,");
  fflush(stdout);
  for (int i = 1; i < 256; i++) {
    printf("%d,", find_hash_mod(i));
    fflush(stdout);
  }
  printf("};");
}

Set inverses_set(u32 value) {
  u32 n;
  if (value > 255) {
    log("Oopsie? value>255 so working hard to find n\n");
    n = find_hash_mod(value);
  } else {
    n = MODS[value];
  }
  Set s = {.n = n};
  u32 v = value;
  for (u32 i = 0;; i++, v = inverseP(v)) {
    u32 key = v % n % SETSZ;
    if (s.thing[key] > 0) {
      // collision!
      if (i > MINLOOKUP) {
        // big enough
        return s;
      } else {
        printf("Invalid mod %d for value 0x%08X", n, value);
        finish(1);
      }
    }
    s.thing[key] = v;
  }
}

int set_has_value(Set *s, u32 value) {
  return s->thing[value % s->n % SETSZ] == value;
}

// end = P^{s}(start) if s = stepsFrom(start, end);
u32 stepsFrom(u32 start, u32 end) {
  u32 v = start;
  for (u32 i = 0;; i++) {
    if (v == end)
      return i;
    v = P(v);
    if (v == start) {
      log("No path from 0x%08X to 0x%08X", start, end);
      finish(1);
    }
  }
}

u32 inversePn(u32 end, u32 n) {
  for (u32 i = 0; i < n; i++) {
    end = inverseP(end);
  }
  return end;
}

/* Zeroes a. Returns the last set position */
u32 zero_a(u32 pos) {
  ms_simple_inner(pos, 0x60);     // b = a
  ms_simple_inner(pos + 2, 0x70); // c = a
  ms_simple_inner(pos + 4, 0xC0); // a = b - c
  return pos + 4;
}

/**
 * ms(setIdx, 0);
 */
void initMemsetZero(u32 setIdx) {
  verbose_printf("initMemsetZero(0x%08X)\n", setIdx);
  initMemsetZeroCount++;
  // .zero_a // zero_a from i to i+4
  // c = a  // at i+6
  // b = B ^ P() // b = b1 = B ^ P(i+9)
  // a = b // a = B ^ P(i+9)
  // padding
  // ms(a, c) // imm before: a = setIdx = P^{stepsFrom(b1, setIdx)}(b1)

  Set inverses = inverses_set(setIdx);
  for (u32 i = 1;; i++) {
    for (u32 B = 0; B < 256; B++) {
      if (B & 0x0F == 0x0F)
        continue;
      u32 b1 = B ^ P(i + 9);
      if (set_has_value(&inverses, b1)) {
        u32 endpt = i + 11 + stepsFrom(b1, setIdx);
        log("i = 0x%08X; endpt = 0x%08X (ms 0)\n", i, endpt);
        // check clean
        gw(endpt);
        if (!rangeIsOpen(i, endpt)) {
          log("nonempty\n");
          continue;
        }
        log("\n--- 0x%08X'ish to 0x%08X: initMemsetZero(0x%08X, 0x%08X)\n\n",
            i - 8, endpt, setIdx, 0);
        zero_a(i);                            // a = 0
        ms_simple_inner(i + 6, 0x70);         // c = a
        ms_simple_inner(i + 8, 0x1F);         // b =
        ms_simple_inner(i + 9, B ^ P(i + 9)); // B ^ P(i + 9);
        ms_simple_inner(i + 11, 0x80);        // a = b
        padding(i + 12, endpt - 2);
        ms_simple_inner(endpt, 0x40); // ms(a, c)
        return;
      }
    no_work:
    }
  }
}

/**
 * ms(setIdx, setValue)
 */
void initMemset(u32 setIdx, u32 setValue) {
  if (setValue == 0)
    return initMemsetZero(setIdx);
  initMemsetCount++;
  verbose_printf("initMemset(0x%08X, 0x%08X)\n", setIdx, setValue);
  // End goal:
  // at idx1: a = inverseP^{i - idx1 + 2 + stepsFrom(b1, setValue)}(setIdx)
  // at i-1: a = inverseP^{2 + stepsFrom(b1, setValue)}(setIdx)
  // then some 0x00s
  // i:   0xFF
  // i+1: 0x1F ^ P(i+1) // b =
  // i+2: B = 0x00 up to 0xFF. Disallow 0x0F mod 0x10 for now.
  //                           Don't want to deal with adding padding
  // at i + 2: b = b1 = B ^ P(i + 2)
  // then some 0x00s
  // at i + 2 + stepsFrom(b1, setValue): b = setValue

  Set inverses = inverses_set(setValue);
  // start at 25 to give some space for force_a_value
  for (u32 i = 25;; i++) {
    for (u32 B = 0; B < 256; B++) {
      if (B & 0x0F == 0x0F)
        continue;
      u32 b1 = B ^ P(i + 2);
      if (set_has_value(&inverses, b1)) {
        u32 endpt = i + 3 + stepsFrom(b1, setValue);
        log("i = 0x%08X; endpt = 0x%08X\n", i, endpt);
        // check clean
        gw(endpt);
        if (!rangeIsOpen(i - 25, endpt)) {
          log("nonempty\n");
          continue;
        }
        log("\n--- 0x%08X'ish to 0x%08X: initMemset(0x%08X, 0x%08X)\n\n",
            i - 23, endpt, setIdx, setValue);
        force_a_value_inner(i - 1, inversePn(setIdx, endpt - (i - 1) - 4));
        ms_simple_inner(i + 1, 0x1F);         // b =
        ms_simple_inner(i + 2, B ^ P(i + 2)); // B ^ P(i + 2);
        padding(i + 3, endpt - 2);
        ms_simple_inner(endpt, 0x30); // ms(a, b)
        return;
      }
    no_work:
    }
  }
}

void ms_smart(u32 idx, u32 v) {
  gw(idx);
  if (can_ms_simple(idx, v))
    return ms_simple(idx, v);
  return initMemset(idx, v);
}

void prep_labels() { labelsFile = fopen("bin/labels", "w"); }

Label *get_label(char *name) {
  for (u32 i = 0; i < num_labels; i++) {
    if (strcmp(labels[i].name, name) == 0) {
      return &(labels[i]);
    }
  }
  return NULL;
}

u32 get_label_pos(char *name) {
  Label *label = get_label(name);
  if (label == NULL) {
    log("Undefined label: %s\n", name);
    finish(1);
  }
  return label->pos;
}

void add_label(u32 idx, char *name) {
  if (get_label(name) != NULL) {
    // Assume this is just a label defined from the label pass
    return;
  }
  labels = realloc(labels, ++num_labels * sizeof(*labels));
  labels[num_labels - 1] = (Label){idx, name};
  fprintf(labelsFile, "0x%08X %s\n", idx, name);
  fflush(labelsFile);
}

Mnemonic mnemonics[] = {
    {0x0F, "a ="},
    {0x1F, "b ="},
    {0x2F, "c ="},
    {0x3F, "ip ="},
    {0x4F, "if (a <= b) ip ="},
    {0x5F, "if (b >= c) ip ="},
    {0x6F, "a = sse(...)"},
    {0x00, "ms(ip, a)"},
    {0x10, "ms(ip, b)"},
    {0x20, "ms(ip, c)"},
    {0x30, "ms(a, b)"},
    {0x40, "ms(a, c)"},
    {0x50, "ms(a, mg(a))"},
    {0x60, "b = a"},
    {0x70, "c = a"},
    {0x80, "a = b"},
    {0x90, "a = c"},
    {0xA0, "ip = a"},
    {0xB0, "a = b + c"},
    {0xC0, "a = b - c"},
    {0xD0, "putchar(a)"},
    {0xE0, "a = getchar()"},
    {0xF0, "exit(0)"},
};

u32 mnemonic_value(char *s) {
  for (u32 i = 0; i < LEN(mnemonics); i++) {
    if (strcmp(mnemonics[i].name, s) == 0)
      return mnemonics[i].value;
  }
  printf("Mnemonic not found: %s\n", s);
  finish(1);
}

bool is_label(char *line) {
  for (; *line; line++) {
    if (*line == ':')
      return true;
  }
  return false;
}

bool has_non_space(char *s) {
  for (; *s; s++) {
    if (*s != ' ')
      return true;
  }
  return false;
}

void remove_trailing_colon(char *name) {
  for (; *name; ++name) {
    if (*name == ':')
      *name = '\0';
  }
}

void advance_past_spaces(char **s_ptr) {
  while (**s_ptr == ' ') {
    (*s_ptr)++;
  }
}

bool starts_with(char *s, char *prefix) {
  return strncmp(prefix, s, strlen(prefix)) == 0;
}

u32 read_raw_value(char *s) {
  advance_past_spaces(&s);
  if (*s == '\0') {
    SADGE("Empty string value");
  } else if (*s == '`') {
    return inverseP(read_raw_value(s + 1));
  } else if (*s == '&')
    return get_label_pos(s + 1);
  else if (*s == '\'') {
    char c = *++s;
    if (c == '\\')
      SADGE("Char literal escapes not supported")
    return c;
  } else if (starts_with(s, "0x")) {
    u32 val;
    u32 cnt = sscanf(s, "%x", &val);
    if (cnt != 1)
      SADGE("Invalid hex literal");
    return val;
  } else if ('0' <= *s && *s <= '9') {
    u32 val;
    u32 cnt = sscanf(s, "%d", &val);
    if (cnt != 1)
      SADGE("Invalid decimal literal");
    return val;
  }
  log("Invalid value: %s\n", s);
  finish(1);
};

u32 read_value(char *s) {
  advance_past_spaces(&s);
  char *left, *right;
  if (sscanf(s, "%ms + %ms", &left, &right) == 2) {
    u32 val = read_raw_value(left) + read_raw_value(right);
    free(left);
    free(right);
    return val;
  } else if (sscanf(s, "%ms - %ms", &left, &right) == 2) {
    u32 val = read_raw_value(left) - read_raw_value(right);
    free(left);
    free(right);
    return val;
  } else {
    return read_raw_value(s);
  }
}

void my_getline(FILE *in, char **line) {
  u32 len = 0;
  while (!feof(in)) {
    char c = fgetc(in);
    len++;
    *line = realloc(*line, len);
    if (c == '\n') {
      (*line)[len - 1] = 0;
      return;
    }
    (*line)[len - 1] = c;
  }
}

#define ENSURE_NEXT_POS                                                        \
  (next_pos == NO_NEXT_POS ? next_pos = last_pos + 1 : next_pos)

/**
 * Go through the file and parse
 * If is_label_pass: just set labels with fixed values, for the purpose
 *  of forward references.
 * Otherwise: Do everything
 */
void _load_from_file(char *filename, bool is_label_pass) {
  inputFile = fopen(filename, "r");
  // The last filled in pos
  u32 last_pos = 0;
  // The next position, as dictated by label or something
  u32 next_pos = NO_NEXT_POS;
  char *line_malloc = malloc(10);
  while (!feof(inputFile)) {
    my_getline(inputFile, &line_malloc);
    if (feof(inputFile))
      break;
    char *line = line_malloc;
    // Line options:
    // comment, starts with //
    // label, label with pos
    //    p:
    //    p: 0x4a00
    // .forceA [value]
    // .trash --> any value really
    // .zero_a --> PPP; a = 0
    // .val [value] --> precisely that value. golfs if preceded by .trash
    // .putchar [value] --> PPP; putchar(value & 255)
    // mnemonic --> sugar for .val
    //    a = getchar()
    //    b = a
    advance_past_spaces(&line);
    // printf("Line: %s\n", line);
    if (*line == '\0' || *line == ';' || *line == '@' ||
        starts_with(line, "//") || starts_with(line, "# ")) {
      // comment or empty line, do nothing
    } else if (*line == '#') {
      SADGE("Expected all macros to be removed (e.g. #define)")
    } else if (is_label(line)) {
      char *colon = strchr(line, ':');
      *colon = '\0';
      char *name = malloc(strlen(line) + 1);
      strcpy(name, line);
      u32 pos;
      bool isAbsolutePositioned = has_non_space(colon + 1);
      if (isAbsolutePositioned) {
        pos = next_pos = read_value(colon + 1);
      } else {
        // TODO: this might sometimes be off by one?
        // Keep a char *pending_label and only add_label when deciding the
        // actual position, but this would break the label pass / forward refs
        pos = last_pos + 1;
      }
      if (!is_label_pass || isAbsolutePositioned)
        add_label(pos, name);
    } else if (is_label_pass) {
      // do nothing; we're just looking for labels with positions defined
    } else if (starts_with(line, ".zero_a")) {
      ENSURE_NEXT_POS;
      if (!can_ms_simple(next_pos, 0x60))
        ++next_pos;
      last_pos = zero_a(next_pos);
      next_pos = NO_NEXT_POS;
    } else if (starts_with(line, ".putchar")) {
      u32 val = read_value(line + sizeof(".putchar") - 1);
      ENSURE_NEXT_POS;
      last_pos = force_putchar(next_pos, val);
      next_pos = NO_NEXT_POS;
    } else if (starts_with(line, ".forceA")) {
      if (next_pos != NO_NEXT_POS)
        SADGE("Can't force A with next pos fixed");
      u32 val = read_value(line + sizeof(".forceA") - 1);
      // 30 is plenty of padding. Could probably easily go down to 25
      force_a_value(last_pos + 32, val);
      last_pos += 33;
      next_pos = NO_NEXT_POS;
    } else if (starts_with(line, ".trash")) {
      if (next_pos != NO_NEXT_POS) {
        last_pos = next_pos;
        next_pos = NO_NEXT_POS;
      } else {
        last_pos += 1;
      }
    } else {
      u32 val;
      if (starts_with(line, ".val")) {
        val = read_value(line + sizeof(".val") - 1);
      } else {
        val = mnemonic_value(line);
      }
      ENSURE_NEXT_POS;
      ms_smart(next_pos, val);
      last_pos = next_pos;
      next_pos = NO_NEXT_POS;
    }
  }
  free(line_malloc);
}

void load_from_file(char *s) {
  _load_from_file(s, true);
  _load_from_file(s, false);
  emit_from_mem();
}

int main(int argc, char *argv[]) {
  if (argc != 2)
    SADGE("Expect exactly one argument (input program), e.g"
          " ./test sources/hello-world.s")
  build_cache();
  prep_labels();
  load_from_file(argv[1]);
  finish(0);
}
