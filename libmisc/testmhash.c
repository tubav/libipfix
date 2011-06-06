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

int main() {
  char *key = malloc (4),
       *value = malloc (4);
  char *res;
  strncpy(key, "foo", 4);
  strncpy(value, "bar", 4);
 
  printf("test 1: init\n");
  initGlobals();
 
  printf("test 2: count\n");
  printf("\t%i\n",countGlobals());

  printf("test 3: write (%s, %s) ...\n", key, value);
  res=setGlobal(key, value);
  printf("\t...%s\n", res);

  printf("test 4: count\n");
  printf("\t%i\n",countGlobals());

  res=0;
  printf("test 5: read foo...\n");
  res=getGlobal(key);
  printf("\t...%s\n", res);

  res=0;
  printf("test 6: read other...\n");
  res=getGlobal("zip");
  printf("\t...%s\n", res);

  res=0;
  printf("test 7: remove other...\n");
  res=removeGlobal("zip");
  printf("\t...%s\n", res);

  printf("test 8: count\n");
  printf("\t%i\n",countGlobals());

  res=0;
  printf("test 9: remove foo...\n");
  res=removeGlobal("foo");
  printf("\t...%s\n", res);

  printf("test 10: count\n");
  printf("\t%i\n",countGlobals());

  printf("test 11: freeing\n");
  freeGlobals();

  return 0;
}
