/*
$$LIC$$
 */
/**************************************************************************
 *
 * mhash.h - miscellaneous hashtable functions.
 *
 * Copyright Fraunhofer FOKUS
 *
 * $Date: 2009-03-27 21:48:11 +0100 (Fri, 27 Mar 2009) $
 *
 * $Revision: 1.2 $
 *
 **************************************************************************/
#ifndef _MHASH_H
#define _MHASH_H

#ifdef   __cplusplus
extern "C" {
#endif

/** get/setGlobal() hashtable 
 */

void     initGlobals();
void*    setGlobal(char *key, void *value);
void*    getGlobal(char *key);
void*    removeGlobal(char *key);
int      countGlobals();
void     freeGlobals();

#ifdef   __cplusplus
}
#endif
#endif 
