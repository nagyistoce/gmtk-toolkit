/*
 * vhash_map.h
 *   General data structure for a vector-stored hash-table implementation
 *   of a map. Each element of the map is a vector.  This is an variation
 *   of vhash_map except the value of the vector is stored.
 *
 * Written by Jeff Bilmes <bilmes@ee.washington.edu>
 * Modified by Gang Ji <gang@ee.washington.edu>
 *
 * Copyright (c) 2003, < fill in later >
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any non-commercial purpose
 * and without fee is hereby granted, provided that the above copyright
 * notice appears in all copies.  The University of Washington,
 * Seattle make no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 *
 * $Header$
 *
 */

#ifndef VSHASH_MAP_H
#define VSHASH_MAP_H

#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "sArray.h"
#include "hash_abstract.h"

// _Key must be a basic type. This is a hash map, mapping from vectors (_Key*)
// which are all the same length (given by object constructor). We map
// from these vectors of _Key's to objects given by _Data.
template <class _Key, class _Data>
class vshash_map : public hash_abstract {
protected:

#ifdef COLLECT_COLLISION_STATISTICS
public:
#endif

	////////////////////////////////////
	// The size of all the vectors in this map must be the
	// same. Here we store the size once, so they don't
	// need to be stored in each element.
	const unsigned vsize;

	////////////////////////////////////////
	// the map "buckets", which include a key mapping to a data item.
	struct MBucket {
		_Key* key;
		_Data item;

		// start off with an empty one.
		MBucket() : key(NULL) {}
		~MBucket() { delete [] key; }
	};

	//////////////////////////////////////////////////////////
	// the actual hash table, an array of pointers to T's
	sArray<MBucket> table;


#ifdef COLLECT_COLLISION_STATISTICS
	unsigned maxCollisions;
	unsigned numCollisions;
	unsigned numInserts;
#endif

	//////////////////////////////////////////////////
	// return true if the two keys are equal.
	bool keyEqual(const register _Key* k1, const register _Key* k2) {
		const register _Key* k1_endp = k1 + vsize;
		do {
			if ( *k1++ != *k2++ )
				return false;
		} while ( k1 != k1_endp );
		return true;
	}

	void keyCopy(_Key* &k1, _Key* &k2) {
		if ( k2 == NULL ) {
			delete [] k1;
			k1 = NULL;
			return;
		}

		if ( k1 == NULL ) {
			k1 = new _Key [vsize];
		}

		const _Key* p1end = k1 + vsize;
		for ( _Key* p1 = k1, *p2 = k2; p1 != p1end; ++p1, ++p2 ) {
			*p1 = *p2;
		}
	}

	///////////////////////////////////////////////////////////////////////
	// since this is a double hash table, we define two address
	// functions, h1() and h2().  h1() gives the starting position of in
	// the array of the key, and h2() gives the increment when we have a
	// collision.
	unsigned h1(const _Key* key, const unsigned arg_size) {
		const _Key* keyp = key + vsize;

		unsigned long a = vsize;
		do {
			--keyp;
			a = 3367900313ul * a + (*keyp) + 1;
		} while ( keyp != key );

		return a % arg_size;
	}

	///////////////////////////////////////////////////////////////////////
	// h2() is the increment of of key when we have a collision.  "The
	// value of h2(key) must be relatively prime to the hash-table m for
	// the entire hash table to be searched" (from Corman, Leiserson,
	// Rivest). Therefore, we have result of the h2() function satisfy
	//         1) it must be greater than zero
	//         2) it must be strictly less than table.size()
	unsigned h2(const _Key* key, const unsigned arg_size) {
		unsigned long a=0;
		const _Key* keyp = key + vsize;
		do {
			--keyp;
			a = 3267000013ul * a + (*keyp) + 1;
		} while ( keyp != key );
		return (a % (arg_size - 1)) + 1;
	}

	///////////////////////////////////////
	// return true if the bucket is empty
	bool empty(const MBucket* bucket) {
		return bucket->key == NULL;
	}
	// return true if the bucket is empty
	bool empty(const MBucket& bucket) {
		return bucket.key == NULL;
	}

	//////////////////////////////////////////////////////////////////
	// return the entry of key
	unsigned entryOf(const _Key* key, sArray<MBucket>& a_table) {
		const unsigned size = a_table.size();
		unsigned a = h1(key, size);

#ifdef COLLECT_COLLISION_STATISTICS
		unsigned collisions=0;
#endif

		if ( empty(a_table[a]) || keyEqual(a_table[a].key,key) )
			return a;

		const unsigned inc = h2(key,size);
		do {
#ifdef COLLECT_COLLISION_STATISTICS
			collisions++;
#endif
			a = (a + inc) % size;
		} while ( !empty(a_table[a]) && (! keyEqual(a_table[a].key, key)) );

#ifdef COLLECT_COLLISION_STATISTICS
		if ( collisions > maxCollisions )
			maxCollisions = collisions;
		numCollisions += collisions;
#endif

		return a;
	}


	////////////////////////////////////////////////////////////
	// resize: resizes the table to be new_size.  new_size *MUST* be a
	// prime number, or everything will fail.
	void resize(int new_size) {
		// make a new table (nt),  and re-hash everyone in the
		// old table into the new table.

		// the next table, used for table resizing.
		sArray<MBucket> nt;
		nt.resize(new_size);

		for ( unsigned i = 0; i < table.size(); i++ ) {
			if ( ! empty(table[i]) ) {
				unsigned a = entryOf(table[i].key, nt);
				keyCopy(nt[a].key, table[i].key);
				nt[a].item = table[i].item;
			}
		}

		table.swap(nt);
		nt.clear();
	}

public:

	////////////////////
	// constructor
	//    All entries in this hash table have the same size given
	//    by the argument arg_vsize.
	vshash_map(const unsigned arg_vsize, unsigned approximateStartingSize = hash_abstract::HashTableDefaultApproxStartingSize) : vsize(arg_vsize) {
		// make sure we hash at least one element, otherwise do {} while()'s won't work.
		assert(vsize > 0);

		_totalNumberEntries=0;
		findPrimesArrayIndex(approximateStartingSize);
		initialPrimesArrayIndex = primesArrayIndex;
		// create the actual hash table here.
		resize(HashTable_PrimesArray[primesArrayIndex]);

#ifdef COLLECT_COLLISION_STATISTICS
		maxCollisions = 0;
		numCollisions = 0;
		numInserts = 0;
#endif
	}

	//////////////////////
	// constructor for empty invalid object.
	// WARNING: this will create an invalid object. It is assumed
	// that this object will re-reconstructed later.
	vshash_map() {}


	/////////////////////////////////////////////////////////
	// clear out the table entirely, including deleting
	// all memory pointed to by the T* pointers.
	void clear(unsigned approximateStartingSize = hash_abstract::HashTableDefaultApproxStartingSize) {
		table.clear();
		_totalNumberEntries=0;
		primesArrayIndex=initialPrimesArrayIndex;
		resize(HashTable_PrimesArray[primesArrayIndex]);
	}

#ifdef COLLECT_COLLISION_STATISTICS
	// clear out the statistics
	void clearStats() {
		maxCollisions = numCollisions = numInserts = 0;
	}
#endif

	///////////////////////////////////////////////////////
	// insert an item <key vector,val> into the hash table.  Return a
	// pointer to the data item in the hash table after it has been
	// inserted.  The foundp argument is set to true when the key has
	// been found. In this case (when it was found), the existing _Data
	// item is not adjusted to be val.
	_Data* insert(_Key* key, _Data val, bool&foundp = hash_abstract::global_foundp) {
#ifdef COLLECT_COLLISION_STATISTICS
		numInserts++;
#endif

		// compute the address
		unsigned a = entryOf(key, table);
		if ( empty(table[a]) ) {
			foundp = false;

			keyCopy(table[a].key, key);
			table[a].item = val;

			// time to resize if getting too big.
			// TODO: probably should resize a bit later than 1/2 entries being used.
			if ( ++_totalNumberEntries >= table.size() / 2 ) {
				if ( primesArrayIndex == (HashTable_SizePrimesArray - 1) )
					error("ERROR: Hash table error, table size exceeds max size of %lu", HashTable_PrimesArray[primesArrayIndex]);
				resize(HashTable_PrimesArray[++primesArrayIndex]);
				// need to re-get location
				a = entryOf(key, table);
				assert(! empty(table[a]) );
			}
		} else {
			foundp = true;
		}

		return &table[a].item;
	}

	_Data* insert(_Key* key, _Data val, _Key* &keyPtr, bool&foundp = hash_abstract::global_foundp) {
#ifdef COLLECT_COLLISION_STATISTICS
		numInserts++;
#endif

		// compute the address
		unsigned a = entryOf(key, table);
		if ( empty(table[a]) ) {
			foundp = false;

			keyCopy(table[a].key, key);
			table[a].item = val;

			// time to resize if getting too big.
			// TODO: probably should resize a bit later than 1/2 entries being used.
			if ( ++_totalNumberEntries >= table.size() / 2 ) {
				if ( primesArrayIndex == (HashTable_SizePrimesArray - 1) )
					error("ERROR: Hash table error, table size exceeds max size of %lu", HashTable_PrimesArray[primesArrayIndex]);
				resize(HashTable_PrimesArray[++primesArrayIndex]);
				// need to re-get location
				a = entryOf(key, table);
				assert(! empty(table[a]) );
			}
		} else {
			foundp = true;
		}

		keyPtr = table[a].key;

		return &table[a].item;
	}


	////////////////////////////////////////////////////////
	// search for key returning true if the key is found, otherwise
	// don't change the table. Return pointer to the data item
	// when found, and NULL when not found.
	_Data* find(_Key* key) {
		const unsigned a = entryOf(key, table);
		if ( ! empty(table[a]) )
			return &table[a].item;
		else
			return NULL;
	}


	/////////////////////////////////////////////////////////
	// search for key, returning a reference to the data item
	// regardless if the value was found or not. This will
	// insert the key into the hash table (similar to the
	// way STL's operator[] works with the hash_set.h and hash_map.h)
	_Data& operator[](_Key* key) {
		_Data* resp = insert(key, _Data());
		// return a reference to the location regardless
		// of if it was found or not.
		return *resp;
	}

#if 0
	// Make these avaialble only in derived classes.
	// Direct access to tables and keys (made available for speed).
	// Use sparingly, and only if you know what you are doing.
	_Key*& tableKey(const unsigned i) { return table[i].key; }
	_Data& tableItem(const unsigned i) { return table[i].item; }
	bool tableEmpty(const unsigned i) { return empty(table[i]); }
	unsigned tableSize() { return table.size(); }
#endif
};


#endif // defined VHASH_MAP2
