#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#if !defined(HASHTABLE)
#define HASHTABLE

#define DEFAULT_HTABLE_BUCKETS 64
#define DEFAULT_BUCKET_SLOTS 4
#define MAX_CUCKOOS 200

#define HASH_BITS uint32_t

struct bch_llist_slot
{
    void *data;
    struct bch_llist_slot *next;
    HASH_BITS hash;
};

struct bch_llist_bucket
{
    size_t slots_used;
    struct bch_llist_slot *slot;
};

struct bch_buckets
{
    struct bch_llist_bucket *bucket;
    HASH_BITS (*hash_f)(const char *key);
};

struct bch_table_info
{
    size_t used;
    size_t buckets;
    size_t bucket_slots;
}; 

struct bch_table
{
    size_t table_count;
    struct bch_buckets *tables;
    struct bch_table_info info;
};

struct bch_table *make_bch_table(
    size_t buckets, size_t bucket_slots,
    size_t table_count, ...);

struct bch_table *rehash_bch_table(
    struct bch_table *table,
    size_t buckets, size_t bucket_slots
);

struct bch_llist_slot *insert_bch_table(
    struct bch_table *table, const char *key, 
    void* value, bool force
);

struct bch_llist_slot *find_bch_table(
    struct bch_table *table, const char *key
);

struct bch_llist_bucket *remove_bch_table(
    struct bch_table *table, const char *key
);

bool destroy_bch_table(
    struct bch_table *table
);

// STDH TABLE

struct stdh_bucket
{
    HASH_BITS hash;
    uint8_t riches;
    uint8_t bytes;
};

struct stdh_table
{
    size_t buckets;
    size_t used;
    struct stdh_bucket* bucket;
    HASH_BITS (*hash_f)(const char *);
};

struct stdh_table *make_stdh_table(
    size_t buckets, HASH_BITS (*hash_f)(const char *)
);

struct stdh_bucket insert_stdh_table(
    struct stdh_table *table, const char *key, 
    uint8_t value
);

struct stdh_bucket find_stdh_table(
    struct stdh_table *table, const char *key
);

struct stdh_bucket remove_stdh_table(
    struct stdh_table *table,
    const char *key
);

bool destroy_stdh_table(
    struct stdh_table *table
);

struct stdh_table *rehash_stdh_table(
    struct stdh_table *table
);

#ifdef SPECIAL_HASH_UTILS

struct stdh_bucket _insert_index(
    struct stdh_table *table, size_t index, 
    HASH_BITS hash, uint8_t value
);

struct stdh_bucket _remove_index(
    struct stdh_table *table,
    HASH_BITS hash,
    size_t index
);

void print_stdh_table(
    struct stdh_table *table
);

#endif

#endif