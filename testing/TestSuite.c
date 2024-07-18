#include <assert.h>
#include <string.h>
#include <stdio.h>

// #define SPECIAL_HASH_UTILS
#include "../HashAlg.c"
#include "../HashTable.h"

/* reverse:  reverse string s in place */
void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* itoa:  convert n to characters in s */
void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

int main(int argc, char **args)
{
    // Creating and destroy hashtables
    struct stdh_table *table1 = make_stdh_table(1, &fnv_1_hash);
    struct stdh_table *table2 = make_stdh_table(10, &fnv_1_hash);
    struct stdh_table *table3 = make_stdh_table(100, &fnv_1_hash);
    struct stdh_table *table4 = make_stdh_table(33, &fnv_1_hash);
    struct stdh_table *table5 = make_stdh_table(9999999, &fnv_1_hash);
    struct stdh_table *table6 = make_stdh_table(0, &fnv_1_hash);
    struct stdh_table *table7 = make_stdh_table(512, &fnv_1_hash);
    struct stdh_table *table8 = make_stdh_table(1024, &fnv_1_hash);
    struct stdh_table *table9 = make_stdh_table(7301, &fnv_1_hash);

    assert(!table6);

    assert(table1->hash_f != NULL);
    assert(table1->bucket != NULL);
    assert(table1->buckets == 1);
    assert(table1->used == 0);

    assert(table2->hash_f != NULL);
    assert(table2->bucket != NULL);
    assert(table2->buckets == 10);
    assert(table2->used == 0);

    assert(table3->hash_f != NULL);
    assert(table3->bucket != NULL);
    assert(table3->buckets == 100);
    assert(table3->used == 0);

    assert(table4->hash_f != NULL);
    assert(table4->bucket != NULL);
    assert(table4->buckets == 33);
    assert(table4->used == 0);

    assert(table5->hash_f != NULL);
    assert(table5->bucket != NULL);
    assert(table5->buckets == 9999999);
    assert(table5->used == 0);

    assert(table7->hash_f != NULL);
    assert(table7->bucket != NULL);
    assert(table7->buckets == 512);
    assert(table7->used == 0);

    assert(table8->hash_f != NULL);
    assert(table8->bucket != NULL);
    assert(table8->buckets == 1024);
    assert(table8->used == 0);

    assert(table9->hash_f != NULL);
    assert(table9->bucket != NULL);
    assert(table9->buckets == 7301);
    assert(table9->used == 0);
    printf("Assertion passed 1\n");

    assert(destroy_stdh_table(table1));
    assert(destroy_stdh_table(table2));
    assert(destroy_stdh_table(table3));
    assert(destroy_stdh_table(table4));
    assert(destroy_stdh_table(table5));
    assert(destroy_stdh_table(table7));
    assert(destroy_stdh_table(table8));
    assert(destroy_stdh_table(table9));

    printf("Assertion passed 2\n");

    //Adding to table 
    table1 = make_stdh_table(97, &fnv_1_hash);
    for (size_t i = 0; i < 97; i++)
    {
        char str[3];
        itoa(i, str);
        insert_stdh_table(table1, str, i, false);
    }
    assert(table1->used == 97);

    table2 = make_stdh_table(1024, &fnv_1_hash);
    for (size_t i = 0; i < 1024; i++)
    {
        char str[5];
        itoa(i, str);
        insert_stdh_table(table2, str, i, false);
    }
    assert(table2->used == 1024);

    table3 = make_stdh_table(20371901, &djb2);
    for (size_t i = 0; i < 20371901; i++)
    {
        char str[10];
        itoa(i, str);
        insert_stdh_table(table3, str, i, false);
    }
    assert(table3->used == 20371901);

    printf("Assertion passed 3\n");

    //Removing from table

    for (size_t i = 0; i < 371901; i++)
    {
        char str[10];
        itoa(i, str);
        struct stdh_bucket check = remove_stdh_table(table3, str);
        assert(check.hash != 0);
    }
    assert(table3->used == 20000000);

    for (size_t i = 0; i < 1024; i++)
    {
        char str[10];
        itoa(i, str);
        struct stdh_bucket check = remove_stdh_table(table2, str);
        assert(check.hash != 0);
    }
    assert(table2->used == 0);

    printf("Assertion passed 4\n");

    //Finding in table

    for (int i = 371902; i < 20371901; i++)
    {
        char str[10];
        itoa(i, str);
        struct stdh_bucket result = find_stdh_table(table3, str); 
        assert(result.bytes == (i % 256));
    }

    struct stdh_bucket nonexistent = find_stdh_table(table3, "clearly false");
    assert(nonexistent.hash == 0);

    printf("Assertion passed 5\n");

    insert_stdh_table(table1, "key1", 1, false);
    insert_stdh_table(table1, "key2", 1, false);
    insert_stdh_table(table1, "key3", 1, false);
    insert_stdh_table(table1, "key4", 1, false);
    insert_stdh_table(table1, "key5", 1, false);

    assert(remove_stdh_table(table1, "key3").hash != 0);
    assert(remove_stdh_table(table1, "key4").hash != 0);
    assert(find_stdh_table(table1, "key5").hash != 0);
    assert(find_stdh_table(table1, "key2").hash != 0);

    assert(find_stdh_table(table1, "key3").hash == 0);
    assert(find_stdh_table(table1, "key4").hash == 0);
    assert(remove_stdh_table(table1, "key1").hash != 0);
    assert(remove_stdh_table(table1, "key5").hash != 0);
    assert(remove_stdh_table(table1, "key2").hash != 0);

    printf("Assertion passed 6\n");

    for (size_t i = 0; i < 54; i++)
    {
        char str[3];
        itoa(i, str);
        remove_stdh_table(table1, str);
    }
    assert(table1->used == 43);
    rehash_stdh_table(table1, 0.75);
    assert(table1->used == 43);

    destroy_stdh_table(table1);
    destroy_stdh_table(table2);
    destroy_stdh_table(table3);

    printf("Assertion passed 7\n");

    printf("All tests done\n");

    return 0;
}