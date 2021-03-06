/**
 * @file ndn/hashtb.h
 * 
 * Hash table.
 *
 * Part of the NDNx C Library.
 *
 * Portions Copyright (C) 2013 Regents of the University of California.
 * 
 * Based on the CCNx C Library by PARC.
 * Copyright (C) 2008, 2009, 2013 Palo Alto Research Center, Inc.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation.
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details. You should have received
 * a copy of the GNU Lesser General Public License along with this library;
 * if not, write to the Free Software Foundation, Inc., 51 Franklin Street,
 * Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef NDN_HASHTB_DEFINED
#define NDN_HASHTB_DEFINED

#include <stddef.h>
#include "common.h"

#define DATA(ht, p) ((void *)((p) + 1))
#define KEY(ht, p) ((unsigned char *)((p) + 1) + ht->item_size)

#define CHECKHTE(ht, hte) ((uintptr_t)((hte)->priv[1]) == ~(uintptr_t)(ht))
#define MARKHTE(ht, hte) ((hte)->priv[1] = (void*)~(uintptr_t)(ht))

struct Attributes {
  int frequency;
  int scope;
  int type;
  int value;
  int defined;
  int declared;
  int array[MAX_DIMENSION];
  int function[MAX_FORMAL_SIZE][MAX_DIMENSION+1];
};

typedef struct Attributes Attributes;

struct exp { int type; int dimension; };

struct Node {
    struct Node* link;
    size_t hash;
    size_t keysize;
    size_t extsize;
    /* user data follows immediately, followed by key */
};

typedef struct Node Node;

struct Hashtb_enumerator;
typedef void (*hashtb_finalize_proc)(struct Hashtb_enumerator *);
struct Hashtb_param {
    hashtb_finalize_proc finalize; /* default is NULL */
    void *finalize_data;           /* default is NULL */
    int orders;                    /* default is 0 */
}; 

typedef struct Hashtb_param Hashtb_param;

struct Hashtb {
    Node **bucket;
    size_t item_size;           /* Size of client's per-entry data */
    unsigned n_buckets;
    int n;                      /* Number of entries */
    int refcount;               /* Number of open enumerators */
    Node *deferred;      /* deferred cleanup */
    Hashtb_param param;  /* saved client parameters */
};

typedef struct Hashtb Hashtb;

/* The client owns the memory for an enumerator, normally in a local. */ 
struct Hashtb_enumerator {
    Hashtb *ht;
    const void *key;        /* Key concatenated with extension data */
    size_t keysize;
    size_t extsize;
    void *data;
    size_t datasize;
    void *priv[3];
};

/*
 * This is a stack designed specifically for holding symbol table
 * it can hold a maximum of MAX_STACK_SIZE symbol tables
 * MAX_STACK_SIZE can be found in common.h
 * pushing a Hashtb into Stack is not allowed
 * push (stack) simply add a new empty hash
 */
struct Stack {
    int top, front;
    size_t item_size;
    Hashtb tables[MAX_STACK_SIZE];
} stack;

typedef struct Stack Stack;

void
stack_init (Stack* stack, size_t itemsize);

int
push (Stack* stack);

void
pop (Stack* stack);

typedef struct Hashtb_enumerator Hashtb_enumerator;

/*
 * hashtb_hash: Calculate a hash for the given key.
 */
size_t
hashtb_hash(const unsigned char *key, size_t key_size);

/*
 * hashtb_create: Create a new hash table.
 * The param may be NULL to use the defaults, otherwise
 * a copy of *param is made.
 */
Hashtb *
hashtb_create(size_t item_size, const Hashtb_param *param);

/*
 * hashtb_get_param: Get the parameters used when creating ht.
 * Return value is the finalize_data; param may be NULL if no
 * other parameters are needed.
 */
void *
hashtb_get_param(Hashtb *ht, Hashtb_param *param);

/*
 * hashtb_destroy: Destroy a hash table and all of its elements.
 */
void
hashtb_destroy(Hashtb **ht);

/*
 * hashtb_n: Get current number of elements.
 */
int
hashtb_n(Hashtb *ht);

/*
 * hashtb_lookup: Find an item
 * Keys are arbitrary data of specified length.
 * Returns NULL if not found, or a pointer to the item's data.
 */
void *
hashtb_lookup(Hashtb *ht, const void *key, size_t keysize);

/*
 * hashtb_start: initializes enumerator to first table entry
 * Order of enumeration is arbitrary.
 * Must do this before using the enumerator for anything else,
 * and must call hashtb_end when done.
 * Returns second argument.
 */
Hashtb_enumerator *
hashtb_start(Hashtb *, Hashtb_enumerator *);
void hashtb_end(Hashtb_enumerator *);

int hashtb_next(Hashtb_enumerator *);

/*
 * hashtb_seek: Find or add an item
 * For a newly added item, the keysize bytes of key along
 * with the extsize following bytes get copied into the
 * hash table's data.  If the key is really a null-terminated
 * string, consider using extsize = 1 to copy the terminator
 * into the keystore.  This feature may also be used to copy
 * larger chunks of unvarying data that are meant to be kept with key.
 *
 * returns 0 if entry existed before, 1 if newly added,
 *        -1 for a fatal error (ENOMEM or key==NULL).
 */
int
hashtb_seek(Hashtb_enumerator *hte,
            const void *key, size_t keysize, size_t extsize);
#define HT_OLD_ENTRY 0
#define HT_NEW_ENTRY 1

/*
 * hashtb_delete: Delete an item
 * The item will be unlinked from the table, and will
 * be freed when safe to do so (i.e., when there are no other
 * active enumerators).  The finalize proc (if any) will be
 * called before the item is freed, and it is responsible for
 * cleaning up any external pointers to the item.
 * When the delete returns, the enumerator will be positioned
 * at the next item.
 */
void hashtb_delete(Hashtb_enumerator *);

/*
 * hashtb_rehash: Hint about number of buckets to use
 * Normally the implementation grows the number of buckets as needed.
 * This optional call might help if the caller knows something about
 * the expected number of elements in advance, or if the size of the
 * table has shrunken dramatically and is not expected to grow soon.
 * Does nothing if there are any active enumerators.
 */
void hashtb_rehash(Hashtb *ht, unsigned n_buckets);

#endif
