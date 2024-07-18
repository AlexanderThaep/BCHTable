#include <stdlib.h>
#include <stdint.h>

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

uint32_t super_fast_hash(const char *str)
{
    return SuperFastHash(str, strlen(str));
}

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

uint32_t crc32(const char *str)
{
    uint32_t crc = ~0U;
    uint8_t c;
    while ((c = *str++))
    {
        crc ^= c;
        for (int i = 0; i < 8; i++)
        {
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
        }
    }
    return (~crc);
}