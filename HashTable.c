#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <HashTable.h>

// TODO: implement this paper https://cseweb.ucsd.edu//~tullsen/horton.pdf

/*
First, str is dereferenced to get the character it points to. This character is 
assigned to c. str is then advanced to the next character. Finally (and this is the part you care about),
the value in c is checked against the null character, which is the test condition for the while loop. 
*/

// This hash works on 32-bit so make sure uint32_t

uint32_t djb2(const char *str)
{
    uint32_t hash = 5381;
    uint32_t c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

uint32_t djb2_xor(const char *str)
{
    uint32_t hash = 5381;
    uint32_t c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) ^ c; /* hash * 33 ^ c */
    return hash;
}

uint32_t djb2_hash(const char *str)
{
    uint32_t hash = 5381;
    uint8_t *s = (uint8_t *) str;
    uint32_t c = *s;
    while (c)
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        s++;
        c = *s;
    }
    return hash;
}

uint32_t djb2_xor_hash(const char *str)
{
    uint32_t hash = 5381;
    uint8_t *s = (uint8_t *) str;
    uint32_t c = *s;
    while (c)
    {
        hash = ((hash << 5) + hash) ^ c; /* hash * 33 ^ c */
        s++;
        c = *s;
    }
    return hash;
}

/*SACRED CODE*/

// http://www.azillionmonkeys.com/qed/hash.html
// Thanks to Paul Hsiesh

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

uint32_t SuperFastHash (const char * data, int len) {
uint32_t hash = len, tmp;
int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += (signed char)*data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

/*SACRED CODE*/

uint32_t fnv_1_hash(const char *str)
{
    uint32_t hash = 0x811c9dc5;
    uint8_t c;
    while ((c = *str++))
    {
        hash *= 0x01000193;
        hash ^= c;
    }
    return hash;
}

uint32_t fnv_1a_hash(const char *str)
{
    uint32_t hash = 0x811c9dc5;
    uint8_t c;
    while ((c = *str++))
    {
        hash ^= c;
        hash *= 0x01000193;
    }
    return hash;
}

// HASHTABLE STUFF BELOW

struct bch_table *make_bch_table(
    size_t buckets, size_t bucket_slots,
    size_t table_count, ...)
{
    struct bch_table *table = malloc(sizeof(*table));
    if (!table) exit(EXIT_FAILURE);

    table->info.used = 0;
    table->info.buckets = buckets;
    table->info.bucket_slots = bucket_slots;
    table->table_count = table_count;
    table->tables = malloc(sizeof(struct bch_buckets) * table_count);
    if (!table->tables) exit(EXIT_FAILURE);

    va_list ptr;
    va_start(ptr, table_count);
    for (size_t i = 0; i < table_count; i++)
    {
        HASH_BITS (*hash_f)(const char *) = va_arg(ptr, HASH_BITS (*)(const char *));
        table->tables[i].hash_f = hash_f;
        table->tables[i].bucket = malloc(sizeof(struct bch_llist_bucket) * buckets);
        if (!table->tables[i].bucket) exit(EXIT_FAILURE);

        table->tables[i].bucket->slot = NULL;
        table->tables[i].bucket->slots_used = 0;
    }
    va_end(ptr);
    return table;
}

static inline struct bch_llist_slot *_bch_find(
    struct bch_llist_bucket *buckets, size_t index, const char *key)
{
    for (struct bch_llist_slot *p = buckets[index].slot; p; p = p->next)
        if (strncmp(key, p->key, strlen(key)) == 0) return p;
    return NULL;
}

struct bch_llist_slot *find_bch_table(
    struct bch_table *table, const char *key)
{
    for (size_t i = 0; i < table->table_count; i++)
    {
        struct bch_buckets *current = &table->tables[i];
        size_t index = (current->hash_f)(key) % table->info.buckets;
        struct bch_llist_slot *s = _bch_find(current->bucket, index, key);
        if (s) return s;
    }
    return NULL;
}

static inline void swap_ptr(void **a, void **b)
{
    void *temp = *a;
    *a = *b;
    *b = temp;
}

static inline void _swap_slots(struct bch_llist_slot *a, struct bch_llist_slot *b)
{
    b->next = a->next;
    a->next = b;

    swap_ptr(&b->data, &a->data);
    swap_ptr(&b->key, &a->key);
}

static inline struct bch_llist_slot *make_bch_slot(void *data, const char *key)
{
    struct bch_llist_slot *slot = malloc(sizeof(*slot));
    if (!slot) exit(EXIT_FAILURE);

    slot->data = data;
    slot->key = strndup(key, strlen(key));
    slot->next = NULL;
    return slot;
}

static inline struct bch_llist_slot *_bch_insert(
    struct bch_table *table, struct bch_llist_bucket *buckets,
    size_t index, struct bch_llist_slot *slot)
{
    table->info.used++;
    struct bch_llist_slot *p = buckets[index].slot;
    struct bch_llist_slot *d = slot;
    if (p)
    {
        _swap_slots(p, d);
    }
    else
    {
        buckets[index].slot = d;
        p = d;
    }
    return p;
}

static inline struct bch_llist_slot *_pop_last(struct bch_llist_slot *head)
{
    struct bch_llist_slot *prev = head;
    struct bch_llist_slot *h = head;
    for (; h->next; h = h->next) prev = h;
    prev->next = NULL;
    return h;
}

struct bch_llist_slot *insert_bch_table(
    struct bch_table *table, const char *key, void* value)
{
    struct bch_llist_slot *p;
    if ((p = find_bch_table(table, key))) return p;

    size_t cycles = 0;
    size_t t_index = 0;
    struct bch_llist_slot *result;
    p = make_bch_slot(value, key);
    while (p)
    {
        struct bch_buckets *current = &table->tables[t_index];
        size_t index = (current->hash_f)(key) % table->info.buckets;
        struct bch_llist_bucket *b = &current->bucket[index];
        if (b->slots_used < 1)
        {
            result = _bch_insert(table, b, index, p);
            break;
        }
        if (cycles >= table->info.buckets)
        {
            if (b->slots_used < table->info.bucket_slots) {
                result = _bch_insert(table, b, index, p);
                break;
            }
            result = _bch_insert(table, b, index, p);
            p = _pop_last(b->slot);
        }
        t_index = (t_index + 1) % table->info.buckets;
        cycles++;
    }
    return result;
}