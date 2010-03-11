/*

 */
/**************************************************************************
 *
 * mhash.c - miscellaneous hashtable functions.
 *
 * Copyright Fraunhofer FOKUS
 *
 * $Date: 2009-03-27 21:48:11 +0100 (Fri, 27 Mar 2009) $
 *
 * $Revision: 1.2 $
 *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>

#include "misc.h"
#include "hashtable.h"

static unsigned int dj2b_hash_from_char_fn( void *strp ) { /* char* */
  char * str = (char*) strp;
  unsigned int hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  return hash;
}

static int char_equal_fn ( void *key1, void *key2 ) { /* char* x2 */
  int l1, l2;
  if ((!key1)||(!key2)) return 0;
  l1 = strlen(key1); 
  l2 = strlen(key2);
  return l1!=l2?0:strncmp(key1, key2, strlen(key1))==0;
}

void initGlobals() {
  ht_globals = create_hashtable(16, dj2b_hash_from_char_fn, char_equal_fn);
}

void* setGlobal(char *key, void *value) {
  if (! hashtable_insert(ht_globals, key, value) ) {
     return 0;
  }
  return value;
}

void* getGlobal(char *key) {
  void *found;
  if (NULL == (found = hashtable_search(ht_globals, key) )) {
    return 0;
  }
  return found;
}

void* removeGlobal(char *key) {
  void *found;
  if (NULL == (found = hashtable_remove(ht_globals, key) )) {
    return 0;
  }
  return found;
}

int countGlobals() {
  return hashtable_count(ht_globals);
}

void freeGlobals() {
  hashtable_destroy(ht_globals, 1); /* second arg indicates "free(value)", might have to be set to zero if necessary */
}
