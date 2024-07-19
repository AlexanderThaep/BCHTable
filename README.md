This is my naive implementation of a **"bucketized cuckoo hash table"** (BCHT) 
utilizing a variable amount of interchangeable hash algorithms that I thought to include in the repository.

They are the FNV-1 and FNV-1a, SuperFastHash, djb2 and djb2-xor algorithms.
The project originally started out as a necessity for a simple hash table to store reserved words for a compiler.
However, as with many projects, it grew in scope until I was scouring the internet for the most bizarre, exotic, and cutting edge
hash functions. In tandem with these, there must be, as a matter of protocol, an equally interesting variant of the hash table that
would utilize these functions.

Luckily for my dwindling sanity, I ended only getting to the research phase and never tried to implement any research paper function from
scratch in C. I stuck with some reasonable hashing functions and lifted the SuperFastHash shamelessly from Paul Hsieh's website.

I was planning to implement the Horton hash table but I ended up just doing the BCHT as an exercise in coding style and knowledge of algorithms
and data structures.

So began my journey, and culminated it in the BHCT that offers support for size_t # of hash table sub-tables (with size_t # of unique hash_functions), 
each with size_t # of hash table buckets, each with size_t # of slots.

For more information about BHCTs, refer to the paper and Wikipedia: 
[https://cseweb.ucsd.edu//~tullsen/horton.pdf](url), [https://en.wikipedia.org/wiki/Cuckoo_hashing](url)

To use it, simply compile it and include HashTable.h
The source should be self-documenting, a few points:
  1. Make sure that values are allocated somewhere before they are passed into the table
  2. For using your own hash functions, make sure they have the correct signature (uint32_t) (*)(const char *)
  3. The use of void pointers is worrisome
  4. Use at your own risk

Added an implementation of robinhood hashing for testiing purposes against the BCHT.
