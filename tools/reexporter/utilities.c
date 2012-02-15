#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <ipfix.h>
#include <misc.h>

#include "utilities.h"

struct templates* appendTemplate(struct templates **list, ipfix_template_t* oldt, ipfix_template_t* newt) {
    struct templates *newTemplate;

    newTemplate = malloc(sizeof(struct templates));
    
    newTemplate->oldt = oldt;
    newTemplate->newt = newt;
    newTemplate->next = NULL;

    newTemplate->next = *list;
    *list = newTemplate;

    return newTemplate;
}

struct templates* getTemplate(struct templates *list, ipfix_template_t* oldt) {
    struct templates *templates_iter = list;

    if(templates_iter == NULL)
        return NULL;

    while (templates_iter != NULL) {
        if(templates_iter->oldt == oldt)
	    return templates_iter;
	templates_iter = templates_iter->next;
    }
    return NULL;
}

void dropTemplates(struct templates **list) {
    struct templates *t_iter = *list;
    struct templates *temp;

    if (t_iter == NULL)
        return;

    while (temp != NULL) {
        temp = t_iter->next;
        free(t_iter);
        t_iter = temp;
    }

    *list = NULL;
}
