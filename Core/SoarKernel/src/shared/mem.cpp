/*************************************************************************
 *
 *  file:  mem.cpp
 *
 * ====================================================================
 * Memory allocation and deallocation, string, linked list, and resizable
 * hash table utilities.
 * Init_memory_utilities() should be called before any of these routines
 * are used.
 * =======================================================================
 */


#include "mem.h"

#include "agent.h"
#include "print.h"

#include <cassert>
#include "run_soar.h"
#include <stdlib.h>

/* ====================================================================

                          String Utilities

   Make_memory_block_for_string() takes a pointer to a string, allocates
   a memory block just large enough to hold the string, and copies the
   string into the block.   Free_memory_block_for_string() frees the
   block.

   "Growable strings" provide a convenient way of building up a string
   piece by piece without having to pre-allocate the right amount of
   memory.  To initialize one, say "gs = make_blank_growable_string()"
   where "gs" is of type "growable_string".  To concatenate a new string
   onto the end of an existing growable string, call
   add_to_growable_string(&gs,new_string) [note the "&"].  When you're
   done using it, call free_growable_string(gs).  Growable strings are
   implemented by allocating a block of memory large enough to hold
   (1) the memory block's size, (2) the current string length, and (3)
   the current text of the string.  Add_to_growable_string() may result
   in a new (larger) memory block being allocated and the text of the
   string being copied.  Three macros provide access to a growable string's
   parts:  memsize_of_growable_string(), length_of_growable_string(),
   and (most importantly) text_of_growable_string(), which is of type
   (char *).
==================================================================== */

char* make_memory_block_for_string(agent* thisAgent, char const* s)
{
    char* p;
    size_t size;

    size = strlen(s) + 1; /* plus one for trailing null character */
    p = static_cast<char*>(thisAgent->memoryManager->allocate_memory(size, STRING_MEM_USAGE));
    strncpy(p, s, size);
    p[size - 1] = 0; /* ensure null termination */
    return p;
}

void free_memory_block_for_string(agent* thisAgent, char* p)
{
    thisAgent->memoryManager->free_memory(p, STRING_MEM_USAGE);
}

#define INITIAL_GROWABLE_STRING_SIZE 100

growable_string make_blank_growable_string(agent* thisAgent)
{
    growable_string gs;

    gs = thisAgent->memoryManager->allocate_memory(2 * sizeof(int*) + INITIAL_GROWABLE_STRING_SIZE,
                         STRING_MEM_USAGE);
    memsize_of_growable_string(gs) = INITIAL_GROWABLE_STRING_SIZE;
    length_of_growable_string(gs) = 0;
    *(text_of_growable_string(gs)) = 0;
    return gs;
}

void add_to_growable_string(agent* thisAgent, growable_string* gs,
                            const char* string_to_add)
{
    size_t current_length, length_to_add, new_length, new_memsize;
    growable_string New;

    current_length = length_of_growable_string(*gs);
    length_to_add = strlen(string_to_add);
    new_length = current_length + length_to_add;
    if (new_length + 1 > static_cast<size_t>(memsize_of_growable_string(*gs)))
    {
        new_memsize = memsize_of_growable_string(*gs);
        while (new_length + 1 > new_memsize)
        {
            new_memsize = new_memsize * 2;
        }
        New = thisAgent->memoryManager->allocate_memory(new_memsize + 2 * sizeof(int*), STRING_MEM_USAGE);
        memsize_of_growable_string(New) = static_cast<int>(new_memsize);
        strcpy(text_of_growable_string(New), text_of_growable_string(*gs));
        thisAgent->memoryManager->free_memory(*gs, STRING_MEM_USAGE);
        *gs = New;
    }
    strcpy(text_of_growable_string(*gs) + current_length, string_to_add);
    length_of_growable_string(*gs) = static_cast<int>(new_length);
}

void free_growable_string(agent* thisAgent, growable_string gs)
{
    thisAgent->memoryManager->free_memory(gs, STRING_MEM_USAGE);
}





/* ====================================================================

                    Cons Cell and List Utilities

   This provides a simple facility for manipulating generic lists, just
   like in Lisp.  A "cons" is a pair of pointers; a "list" is just a cons
   (or NIL).  We maintain a memory pool (see above) of conses.
   Allocate_cons() is like allocate_with_pool() for conses; free_cons()
   is like free_with_pool.  Push(new_item,my_list) is a macro for adding
   a new item onto the front of an existing list.

   In addition to the regular conses, we also have a pool of "dl_cons"es--
   these are like conses, only doubly-linked.  A "dl_list" is a just a
   dl_cons (or NIL).

   Some (consed) list utility functions:  destructively_reverse_list()
   does just what it says, and returns a pointer to the new head of the
   list (formerly the tail).  Member_of_list() tests membership, using
   "==" as the equality predicates.  Add_if_not_member() is like Lisp's
   "pushnew"; it returns the new list (possibly unchanged) list.
   Free_list() frees all the cons cells in the list.

   Sometimes we need to surgically extract particular elements from a
   list.  Extract_list_elements() and extract_dl_list_elements() do this.
   They use a callback function that indicates which elements to extract:
   the callback function is called on each element of the list, and should
   return true for the elements to be extracted.  The two extraction
   functions return a list (or dl_list) of the extracted elements.
==================================================================== */

cons* destructively_reverse_list(cons* c)
{
    cons* prev, *current, *next;

    prev = NIL;
    current = c;
    while (current)
    {
        next = current->rest;
        current->rest = prev;
        prev = current;
        current = next;
    }
    return prev;
}

bool member_of_list(void* item, cons* the_list)
{
    while (the_list)
    {
        if (the_list->first == item)
        {
            return true;
        }
        the_list = the_list->rest;
    }
    return false;
}

cons* add_if_not_member(agent* thisAgent, void* item, cons* old_list)
{
    cons* c;

    for (c = old_list; c != NIL; c = c->rest)
        if (c->first == item)
        {
            return old_list;
        }
    allocate_cons(thisAgent, &c);
    c->first = item;
    c->rest = old_list;
    return c;
}

void free_list(agent* thisAgent, cons* the_list)
{
    cons* c;

    while (the_list)
    {
        c = the_list;
        the_list = the_list->rest;
        free_cons(thisAgent, c);
    }
}

cons* extract_list_elements(agent* thisAgent, cons** header, cons_test_fn f, void* data)
{
    cons* first_extracted_element, *tail_of_extracted_elements;
    cons* c, *prev_c, *next_c;

    first_extracted_element = NIL;
    tail_of_extracted_elements = NIL;

    prev_c = NIL;
    for (c = (*header); c != NIL; c = next_c)
    {
        next_c = c->rest;
        if (!f(thisAgent, c, data))
        {
            prev_c = c;
            continue;
        }

        if (prev_c)
        {
            prev_c->rest = next_c;
        }
        else
        {
            *header = next_c;
        }
        if (first_extracted_element)
        {
            tail_of_extracted_elements->rest = c;
        }
        else
        {
            first_extracted_element = c;
        }

        tail_of_extracted_elements = c;
    }

    if (first_extracted_element)
    {
        tail_of_extracted_elements->rest = NIL;
    }

    return first_extracted_element;
}

dl_list* extract_dl_list_elements(agent* thisAgent, dl_list** header, dl_cons_test_fn f)
{
    dl_cons* first_extracted_element, *tail_of_extracted_elements;
    dl_cons* dc, *next_dc;

    first_extracted_element = NIL;
    tail_of_extracted_elements = NIL;

    for (dc = (*header); dc != NIL; dc = next_dc)
    {
        next_dc = dc->next;

        if (!f(dc, thisAgent))
        {
            continue;
        }

        remove_from_dll((*header), dc, next, prev);

        if (first_extracted_element)
        {
            tail_of_extracted_elements->next = dc;
        }
        else
        {
            first_extracted_element = dc;
        }

        dc->prev = tail_of_extracted_elements;
        tail_of_extracted_elements = dc;
    }

    /************************************************************************/

    if (first_extracted_element)
    {
        tail_of_extracted_elements->next = NIL;
    }

    return first_extracted_element;
}

bool cons_equality_fn(agent*, cons* c, void* data)
{
    return (c->first == data);
}

/* ====================================================================

                   Resizable Hash Table Routines

   We use hash tables in various places, and don't want to have to fix
   their size ahead of time.  These routines provide hash tables that
   are dynamically resized as items are added and removed.  We use
   "open hashing" with a hash table whose size is a power of two.  We
   keep track of how many items are in the table.  The table grows
   when # of items >= 2*size, and shrinks when # of items < size/2.
   To resize a hash table, we rehash every item in it.

   Each item must be a structure whose first field is reserved for use
   by these hash table routines--it points to the next item in the hash
   bucket.  Hash tables are created and initialized via make_hash_table();
   you give it a hash function (i.e., a C function) that finds the hash
   value for a given item, hashing it to a value num_bits wide.  For aid
   in this, we provide masks_for_n_low_order_bits[] that select out the
   low-order bits of a value:  (x & masks_for_n_low_order_bits[23]) picks
   out the 23 low-order bits of x.

   Items are added/removed from a hash table via add_to_hash_table() and
   remove_from_hash_table().  These calls resize the hash table if
   necessary.

   The contents of a hash table (or one bucket in the table) can be
   retrieved via do_for_all_items_in_hash_table() and
   do_for_all_items_in_hash_bucket().  Each uses a callback function,
   invoking it with each successive item.  The callback function should
   normally return false.  If the callback function ever returns true,
   iteration over the hash table items stops and the do_for_xxx()
   routine returns immediately.
==================================================================== */

uint32_t masks_for_n_low_order_bits[33] = { 0x00000000,
                                            0x00000001, 0x00000003, 0x00000007, 0x0000000F,
                                            0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
                                            0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
                                            0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
                                            0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
                                            0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
                                            0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
                                            0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
                                          };

struct hash_table_struct* make_hash_table(agent* thisAgent, short minimum_log2size,
        hash_function h)
{
    hash_table* ht;

    ht = static_cast<hash_table_struct*>(thisAgent->memoryManager->allocate_memory(sizeof(hash_table),
                                         HASH_TABLE_MEM_USAGE));
    ht->count = 0;
    if (minimum_log2size < 1)
    {
        minimum_log2size = 1;
    }
    ht->size = static_cast<uint32_t>(1) << minimum_log2size;
    ht->log2size = minimum_log2size;
    ht->minimum_log2size = minimum_log2size;
    ht->buckets = static_cast<item_in_hash_table_struct**>(thisAgent->memoryManager->allocate_memory_and_zerofill(ht->size * sizeof(char*),
                  HASH_TABLE_MEM_USAGE));
    ht->h = h;
    return ht;
}

void resize_hash_table(agent* thisAgent, hash_table* ht, short new_log2size)
{
    uint32_t i;
    bucket_array* new_buckets;
    item_in_hash_table* item, *next;
    uint32_t hash_value;
    uint32_t new_size;

    new_size = static_cast<uint32_t>(1) << new_log2size;
    new_buckets =
        (bucket_array*) thisAgent->memoryManager->allocate_memory_and_zerofill(new_size * sizeof(char*),
                HASH_TABLE_MEM_USAGE);

    for (i = 0; i < ht->size; i++)
    {
        for (item = *(ht->buckets + i); item != NIL; item = next)
        {
            next = item->next;
            /* --- insert item into new buckets --- */
            hash_value = (*(ht->h))(item, new_log2size);
            item->next = *(new_buckets + hash_value);
            *(new_buckets + hash_value) = item;
        }
    }

    thisAgent->memoryManager->free_memory(ht->buckets, HASH_TABLE_MEM_USAGE);
    ht->buckets = new_buckets;
    ht->size = new_size;
    ht->log2size = new_log2size;
}

/* RPM 6/09 */
void free_hash_table(agent* thisAgent, struct hash_table_struct* ht)
{
    thisAgent->memoryManager->free_memory(ht->buckets, HASH_TABLE_MEM_USAGE);
    thisAgent->memoryManager->free_memory(ht, HASH_TABLE_MEM_USAGE);
}

void remove_from_hash_table(agent* thisAgent, struct hash_table_struct* ht,
                            void* item)
{
    uint32_t hash_value;
    item_in_hash_table* this_one, *prev;

    this_one = static_cast<item_in_hash_table_struct*>(item);
    hash_value = (*(ht->h))(item, ht->log2size);
    if (*(ht->buckets + hash_value) == this_one)
    {
        /* --- hs is the first one on the list for the bucket --- */
        *(ht->buckets + hash_value) = this_one->next;
    }
    else
    {
        /* --- hs is not the first one on the list, so find its predecessor --- */
        prev = *(ht->buckets + hash_value);
        while (prev && prev->next != this_one)
        {
            prev = prev->next;
        }
        if (!prev)
        {
            /* Reaching here means that we couldn't find this_one item */
            assert(prev && "Couldn't find item to remove from hash table!");
            return;
        }
        prev->next = this_one->next;
    }
    this_one->next = NIL;  /* just for safety */
    /* --- update count and possibly resize the table --- */
    ht->count--;
    if ((ht->count < ht->size / 2) && (ht->log2size > ht->minimum_log2size))
    {
        resize_hash_table(thisAgent, ht, ht->log2size - 1);
    }
}

void add_to_hash_table(agent* thisAgent, struct hash_table_struct* ht,
                       void* item)
{
    uint32_t hash_value;
    item_in_hash_table* this_one;

    this_one = static_cast<item_in_hash_table_struct*>(item);
    ht->count++;
    if (ht->count >= ht->size * 2)
    {
        resize_hash_table(thisAgent, ht, ht->log2size + 1);
    }
    hash_value = (*(ht->h))(item, ht->log2size);
    this_one->next = *(ht->buckets + hash_value);
    *(ht->buckets + hash_value) = this_one;
}

void do_for_all_items_in_hash_table(agent* thisAgent,
                                    struct hash_table_struct* ht,
                                    hash_table_callback_fn2 f,
                                    void* userdata)
{
    uint32_t hash_value;
    item_in_hash_table* item;

    for (hash_value = 0; hash_value < ht->size; hash_value++)
    {
        item = (item_in_hash_table*)(*(ht->buckets + hash_value));
        for (; item != NIL; item = item->next)
            if ((*f)(thisAgent, item, userdata))
            {
                return;
            }
    }
}

void do_for_all_items_in_hash_bucket(struct hash_table_struct* ht,
                                     hash_table_callback_fn f,
                                     uint32_t hash_value)
{
    item_in_hash_table* item;

    hash_value = hash_value & masks_for_n_low_order_bits[ht->log2size];
    item = (item_in_hash_table*)(*(ht->buckets + hash_value));
    for (; item != NIL; item = item->next)
        if ((*f)(item))
        {
            return;
        }
}

/* ====================================================================

                       Module Initialization

   This routine should be called before anything else in this file
   is used.
==================================================================== */

void init_memory_utilities(agent* thisAgent)
{
    thisAgent->memoryManager->init_memory_pool(MP_cons_cell, sizeof(cons), "cons cell");
    thisAgent->memoryManager->init_memory_pool(MP_dl_cons, sizeof(dl_cons), "dl cons");
}

