#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <HashTable.h>

// TODO: implement this paper https://cseweb.ucsd.edu//~tullsen/horton.pdf
// HASHTABLE STUFF BELOW

struct bch_table *make_bch_table(
    size_t buckets, size_t bucket_slots,
    size_t table_count, ...)
{
    if (buckets < 1) exit(EXIT_FAILURE);
    if (bucket_slots < 1) exit(EXIT_FAILURE);
    if (table_count < 2) exit(EXIT_FAILURE);

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

        for (size_t j = 0; j < buckets; j++)
        {
            table->tables[i].bucket[j].slot = NULL;
            table->tables[i].bucket[j].slots_used = 0;
        }
    }
    va_end(ptr);
    return table;
}

static inline struct bch_llist_slot *_bch_find(
    struct bch_llist_bucket *buckets, size_t index, const char *key)
{
    for (struct bch_llist_slot *p = buckets[index].slot; p; p = p->next)
        if (strcmp(key, p->key) == 0) return p;
    return NULL;
}

struct bch_llist_slot *find_bch_table(
    struct bch_table *table, const char *key)
{
    if (!table || !key) exit(EXIT_FAILURE);
    
    for (size_t i = 0; i < table->table_count; i++)
    {
        struct bch_buckets *current = table->tables + i;
        size_t index = (current->hash_f)(key) % table->info.buckets;
        struct bch_llist_slot *s = _bch_find(current->bucket, index, key);
        if (s) return s;
    }
    return NULL;
}

static inline void swap_ptr(
    void **a, void **b)
{
    void *temp = *a;
    *a = *b;
    *b = temp;
}

static inline void _swap_slots(
    struct bch_llist_slot *a, struct bch_llist_slot *b)
{
    b->next = a->next;
    a->next = b;

    swap_ptr(&b->data, &a->data);
    swap_ptr(&b->key, &a->key);
}

static inline struct bch_llist_slot *make_bch_slot(
    void *data, const char *key)
{
    if (!data || !key) exit(EXIT_FAILURE);

    struct bch_llist_slot *slot = malloc(sizeof(*slot));
    if (!slot) exit(EXIT_FAILURE);

    slot->data = data;
    slot->key = strndup(key, strlen(key));
    if (!slot->key) exit(EXIT_FAILURE);

    slot->next = NULL;
    return slot;
}

static inline struct bch_llist_slot *_bch_insert(
    struct bch_table *table, struct bch_llist_bucket *buckets,
    struct bch_llist_slot *slot)
{
    table->info.used++;
    buckets->slots_used++;
    struct bch_llist_slot *p = buckets->slot;
    struct bch_llist_slot *d = slot;
    if (p)
    {
        _swap_slots(p, d);
    }
    else
    {
        buckets->slot = d;
        p = d;
    }
    return p;
}

static inline struct bch_llist_slot *_pop_last(
    struct bch_llist_bucket *bucket)
{
    bucket->slots_used--;
    struct bch_llist_slot *prev = bucket->slot;
    struct bch_llist_slot *h = prev;
    for (; h->next; h = h->next) prev = h;
    prev->next = NULL;
    return h;
}

struct bch_llist_slot *insert_bch_table(
    struct bch_table *table, const char *key, void* value)
{
    if (!table || !key || !value) exit(EXIT_FAILURE);
    struct bch_llist_slot *p;
    if ((p = find_bch_table(table, key))) return p;

    size_t pops = 0;
    size_t cycles = 0;
    size_t t_index = 0;
    struct bch_llist_slot *result;
    p = make_bch_slot(value, key);
    for (;p;t_index = (t_index + 1) % table->table_count)
    {
        if (pops > MAX_CUCKOOS)
        {
            fprintf(stdout, 
            "Table is too full and exceeded cuckooing limit!\n");
            exit(EXIT_FAILURE);
        }
        struct bch_buckets *current = table->tables + t_index;
        size_t index = (current->hash_f)(key) % table->info.buckets;
        struct bch_llist_bucket *b = current->bucket + index;
        if (b->slots_used >= 1 && cycles < table->info.buckets)
        {
            cycles++;
            continue;
        }
        if (b->slots_used >= table->info.bucket_slots)
        {
            result = _bch_insert(table, b, p);
            p = _pop_last(b);

            pops++;
            cycles = 0;
            continue;
        }
        result = _bch_insert(table, b, p);
        break;
    }
    return result;
}

struct bch_llist_bucket *remove_bch_table(
    struct bch_table *table, const char *key)
{
    if (!table || !key) exit(EXIT_FAILURE);
    for (size_t i = 0; i < table->table_count; i++)
    {
        struct bch_buckets *current = table->tables + i;
        size_t index = (current->hash_f)(key) % table->info.buckets;
        struct bch_llist_bucket *bucket = current->bucket + index;
        struct bch_llist_slot *s = bucket->slot;
        struct bch_llist_slot *p = s;
        while (s)
        {
            if (strcmp(key, s->key) != 0)
            {
                p = s;
                s = s->next;
                continue;
            }

            if (!s->next) bucket->slot = NULL;
            p->next = s->next;
            free(s->data);
            free(s->key);
            free(s);

            bucket->slots_used--;
            table->info.used--;
            return bucket;
        }
    }
    return NULL;
}

static inline void purge_bucket(
    struct bch_llist_bucket *bucket
)
{
    struct bch_llist_slot *slot = bucket->slot;
    while (slot)
    {
        struct bch_llist_slot *next = slot->next;
        free(slot->data);
        free(slot->key);
        free(slot);
        slot = next;
    }
}

static inline void purge_table(
    struct bch_table *table, struct bch_buckets *bucket
)
{
    for (size_t i = 0; i < table->info.buckets; i++)
        purge_bucket(bucket->bucket + i);
    
    free(bucket->bucket);
}

bool destroy_bch_table(
    struct bch_table *table)
{
    if (!table) return false;
    for (size_t i = 0; i < table->table_count; i++)
    {
        struct bch_buckets *current = table->tables + i;
        purge_table(table, current);
    }
    free(table->tables);
    free(table);
    return true;
}

// STANDARD HASH TABLE (for normal, human purposes)

struct stdh_table {
    size_t buckets;
    size_t bucket_slots;
};

struct stdh_table *make_stdh_table(
    size_t buckets, size_t bucket_slots)
{
}