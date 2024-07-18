#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <HashTable.h>

// TODO: implement this paper https://cseweb.ucsd.edu//~tullsen/horton.pdf
// HASHTABLE STUFF BELOW

static struct bch_table *_spawn_bch_table(
    size_t buckets, size_t bucket_slots,
    size_t table_count)
{
    struct bch_table *table = malloc(sizeof(*table));
    if (!table) exit(EXIT_FAILURE);

    table->info.used = 0;
    table->info.buckets = buckets;
    table->info.bucket_slots = bucket_slots;
    table->table_count = table_count;
    table->tables = malloc(sizeof(struct bch_buckets) * table_count);
    if (!table->tables) exit(EXIT_FAILURE);

    for (size_t i = 0; i < table_count; i++)
    {
        table->tables[i].bucket = malloc(sizeof
            (struct bch_llist_bucket) * buckets);
        if (!table->tables[i].bucket) exit(EXIT_FAILURE);

        for (size_t j = 0; j < buckets; j++)
        {
            table->tables[i].bucket[j].slot = NULL;
            table->tables[i].bucket[j].slots_used = 0;
        }
    }
    return table;
}

struct bch_table *make_bch_table(
    size_t buckets, size_t bucket_slots,
    size_t table_count, ...)
{
    if (buckets < 1) exit(EXIT_FAILURE);
    if (bucket_slots < 1) exit(EXIT_FAILURE);
    if (table_count < 2) exit(EXIT_FAILURE);

    struct bch_table *table = _spawn_bch_table(
        buckets, 
        bucket_slots, 
        table_count);
    if (!table) exit(EXIT_FAILURE);

    va_list ptr;
    va_start(ptr, table_count);
    for (size_t i = 0; i < table_count; i++)
    {
        HASH_BITS (*hash_f)(const char *) = 
            va_arg(ptr, HASH_BITS (*)(const char *));
        table->tables[i].hash_f = hash_f;
    }
    va_end(ptr);
    return table;
}

static inline struct bch_llist_slot *_bch_find(
    struct bch_llist_bucket *buckets, size_t index, 
    HASH_BITS hash)
{
    for (
        struct bch_llist_slot *p = buckets[index].slot; 
        p; 
        p = p->next)

        if (hash == p->hash) return p;
    return NULL;
}

struct bch_llist_slot *find_bch_table(
    struct bch_table *table, const char *key)
{
    if (!table || !key) exit(EXIT_FAILURE);
    
    for (size_t i = 0; i < table->table_count; i++)
    {
        struct bch_buckets *current = table->tables + i;
        HASH_BITS hash = (current->hash_f)(key);
        size_t index = hash % table->info.buckets;
        struct bch_llist_slot *s = _bch_find(
            current->bucket, 
            index, 
            hash);
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
    swap_ptr(&b->hash, &a->hash);
}

static inline struct bch_llist_slot *make_bch_slot(
    void *data, HASH_BITS hash)
{
    struct bch_llist_slot *slot = malloc(sizeof(*slot));
    if (!slot) exit(EXIT_FAILURE);

    slot->data = data;
    slot->hash = hash;

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
    struct bch_llist_slot *prev = bucket->slot;
    struct bch_llist_slot *h;
    for (h = prev; h->next; h = h->next) 
        prev = h;
    prev->next = NULL;
    return h;
}

static struct bch_llist_slot *_nohash_bch_insert(
    struct bch_table *table, HASH_BITS hash,
    void *value, bool force)
{
    struct bch_llist_slot *p = make_bch_slot(value, 0);

    size_t pops = 0;
    size_t cycles = 0;
    size_t t_index = 0;
    struct bch_llist_slot *result;
    for (; p; t_index++)
    {
        t_index %= table->table_count;
        struct bch_buckets *current = table->tables + t_index;
        size_t index = hash % table->info.buckets;
        struct bch_llist_bucket *b = current->bucket + index;
        p->hash = hash;
        if (b->slots_used >= table->info.bucket_slots)
        {
            result = _bch_insert(table, b, p);
            p = _pop_last(b);
            table->info.used--;
            b->slots_used--;

            pops++;
            cycles = 0;
            continue;
        }
        result = _bch_insert(table, b, p);
        pops = 0;
        break;
    }
    return result;
}

struct bch_table *rehash_bch_table(
    struct bch_table *table,
    size_t buckets, size_t bucket_slots)
{
    struct bch_table *new_table = _spawn_bch_table(
        buckets, 
        bucket_slots, 
        table->table_count);

    for (size_t k = 0; k < table->table_count; k++)
    {
        struct bch_buckets *t = table->tables + k;
        new_table->tables[k].hash_f = t->hash_f;
    }
    
    for (size_t i = 0; i < table->table_count; i++)
    {
        struct bch_buckets *t = table->tables + i;
        for (size_t j = 0; j < table->info.buckets; j++)
        {
            struct bch_llist_slot *s = t->bucket[j].slot;
            if (!s) continue;
            while (s)
            {
                struct bch_llist_slot *next = s->next;
                _nohash_bch_insert(
                    new_table, s->hash, 
                    s->data, false);
                s = next;
            }
        }
    }

    return new_table;
}

struct bch_llist_slot *insert_bch_table(
    struct bch_table *table, const char *key, 
    void* value, bool force)
{
    if (!table || !key) exit(EXIT_FAILURE);
    float load_factor = 
        (float) table->info.used / 
        (float) table->info.buckets;

    #ifdef SPECIAL_HASH_UTILS
    if (load_factor > 5.0f)
    {
        fprintf(stdout, 
        "Table dangerously high load factor [%f]!\n", 
        load_factor);
    }
    #endif

    struct bch_llist_slot *p;
    if (!force && (p = find_bch_table(table, key))) return p;
    p = make_bch_slot(value, 0);

    size_t cuckoos = 0;
    size_t t_index = 0;
    struct bch_llist_slot *result;
    for (; p; t_index++)
    {
        t_index %= table->table_count;
        if (cuckoos > MAX_CUCKOOS)
        {
            fprintf(stdout, 
            "Table too full, exceeded cuckooing limit!\n");
            exit(EXIT_FAILURE);
        }
        struct bch_buckets *current = table->tables + t_index;
        HASH_BITS hash = (current->hash_f)(key);
        size_t index = hash % table->info.buckets;
        struct bch_llist_bucket *b = current->bucket + index;
        p->hash = hash;
        if (b->slots_used >= table->info.bucket_slots)
        {
            result = _bch_insert(table, b, p);
            p = _pop_last(b);
            table->info.used--;
            b->slots_used--;

            cuckoos++;
            continue;
        }
        result = _bch_insert(table, b, p);
        cuckoos = 0;
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
        HASH_BITS hash = (current->hash_f)(key);
        size_t index = hash % table->info.buckets;
        struct bch_llist_bucket *bucket = current->bucket + index;
        struct bch_llist_slot *s = bucket->slot;
        struct bch_llist_slot *p = s;
        while (s)
        {
            if (hash != s->hash)
            {
                p = s;
                s = s->next;
                continue;
            }

            if (!s->next) bucket->slot = NULL;
            p->next = s->next;
            free(s->data);
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

struct stdh_table *make_stdh_table(
    size_t buckets, HASH_BITS (*hash_f)(const char *))
{
    if (buckets < 1) return NULL;
    struct stdh_table *table = malloc(sizeof(*table));
    table->hash_f = hash_f;
    table->buckets = buckets;
    table->used = 0;
    table->probe_limit = log2(buckets);
    table->bucket = calloc(sizeof(*table->bucket) * buckets, 1);
    return table;
}

static inline struct stdh_bucket _find_hash(
    struct stdh_table *table, HASH_BITS hash
)
{
    size_t index = hash % table->buckets;
    size_t origin = index;

    struct stdh_bucket b = {};
    if (table->bucket[index].hash == 0) return b;

    while (table->bucket[index].hash != hash)
    {
        if (index >= table->buckets 
            || table->bucket[index].hash == 0
            || index - origin > table->probe_limit) return b;
        index++;
    }
    return table->bucket[index];
}

struct stdh_bucket find_stdh_table(
    struct stdh_table *table, const char *key)
{
    if (!table || !key) exit(EXIT_FAILURE);
    HASH_BITS hash = table->hash_f(key);
    return _find_hash(table, hash);
}

static inline struct stdh_bucket _insert_hash(
    struct stdh_table *table, HASH_BITS hash,
    uint8_t value)
{
    struct stdh_bucket c = {};
    size_t index = hash % table->buckets;
    struct stdh_bucket b = { .bytes = value, .hash = hash};

    while (b.hash != 0)
    {
        if (table->bucket[index].hash == b.hash) 
        {
            uint32_t hashish = table->bucket[index].hash;
            uint8_t valueh = table->bucket[index].bytes;
            return table->bucket[index];
        }
        if (index >= table->buckets || b.riches > table->probe_limit) 
        {
            table = rehash_stdh_table(table, DEFAULT_RESIZE);
            index = b.hash % table->buckets;
            b.riches = 0;
        }
        if (
            table->bucket[index].riches < b.riches ||
            table->bucket[index].hash == 0)
        {
            c = b;
            b = table->bucket[index];
            table->bucket[index] = c;
        }
        b.riches++;
        index++;
    }
    table->used++;
    return c;
}

struct stdh_bucket insert_stdh_table(
    struct stdh_table *table, const char *key, 
    uint8_t value, bool force)
{
    if (!table || !key) exit(EXIT_FAILURE);

    #ifdef SPECIAL_HASH_UTILS
    float load_factor = 
        (float) table->used / 
        (float) table->buckets;
    if (load_factor > 0.8f)
    {
        fprintf(stdout, 
        "Table dangerously high load factor [%f]!\n", 
        load_factor);
    }
    #endif

    HASH_BITS hash = table->hash_f(key);
    struct stdh_bucket c = {};
    // if (!force && (c = _find_hash(table, hash)).hash != 0) return c;
    c = _insert_hash(table, hash, value);
    return c;
}

static inline struct stdh_bucket _remove_hash(
    struct stdh_table *table,
    HASH_BITS hash
)
{
    size_t index = hash % table->buckets;
    struct stdh_bucket b = table->bucket[index];

    while (index < table->buckets && b.hash != hash) 
    {
        if (b.hash == 0) return (struct stdh_bucket) {};
        b = table->bucket[++index];
    }
    
    struct stdh_bucket c = b;
    table->bucket[index] = (struct stdh_bucket) {};
    table->used--;
    
    b = table->bucket[++index];
    while (b.riches > 0)
    {
        if (index >= table->buckets) exit(EXIT_FAILURE);
        table->bucket[index] = (struct stdh_bucket) {};
        b.riches--;
        table->bucket[index - 1] = b;
        b = table->bucket[++index];
    }

    return c;
}

struct stdh_bucket remove_stdh_table(
    struct stdh_table *table,
    const char *key)
{
    HASH_BITS hash = table->hash_f(key);
    return _remove_hash(table, hash);
}

void print_stdh_table(struct stdh_table *table)
{
    printf("\n%*s %*s %*s\n", 10, "[HASH]", 10, "[DATA]", 10, "[RICHES]");
    for (size_t index = 0; index < table->buckets; index++)
    {
        struct stdh_bucket c = table->bucket[index];
        printf("%*x %*u %*u\n", 10, c.hash, 10, c.bytes, 10, c.riches);
    }
    printf("This table has [%u] items!\n", table->used);
}

bool destroy_stdh_table(struct stdh_table *table)
{
    if (!table) return false;
    free(table->bucket);
    free(table);
    return true;
}

struct stdh_table *rehash_stdh_table(struct stdh_table *table, double factor)
{
    size_t old_bucket_size = table->buckets;
    size_t new_bucket_size = (size_t)(((double)table->buckets) * factor);

    struct stdh_bucket *old_buckets = table->bucket;
    table->bucket = calloc(new_bucket_size, sizeof(*table->bucket));

    table->used = 0;
    table->buckets = new_bucket_size;
    table->probe_limit = log2(new_bucket_size);

    for (size_t i = 0; i < old_bucket_size; i++)
    {
        struct stdh_bucket b = old_buckets[i];
        if (b.hash == 0) continue;

        _insert_hash(table, b.hash, b.bytes);
    }

    free(old_buckets);

    return table;
}