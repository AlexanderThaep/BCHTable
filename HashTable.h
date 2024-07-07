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

struct bch_llist_slot *insert_bch_table(
    struct bch_table *table, const char *key, 
    void* value, bool force);

struct bch_llist_slot *find_bch_table(
    struct bch_table *table, const char *key);

struct bch_llist_bucket *remove_bch_table(
    struct bch_table *table, const char *key);

bool destroy_bch_table(
    struct bch_table *table);

#endif