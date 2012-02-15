#ifndef UTLITIES_H
#define UTILITIES_H

struct templates {
    ipfix_template_t* oldt;
    ipfix_template_t* newt;
    struct templates* next;
};

struct templates* appendTemplate(struct templates**, ipfix_template_t*, ipfix_template_t*);
struct templates* getTemplate(struct templates*, ipfix_template_t*);
void dropTemplates(struct templates **);

#endif
