/*
 * $Id$
 */

#ifndef XML_H
#define XML_H

#include <string>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

xmlDocPtr xmlParse(const char *filename);
bool xmlPropExists(xmlNodePtr node, const char *name);
std::string xmlGetPropAsString(xmlNodePtr node, const char *name);
bool xmlGetPropAsBool(xmlNodePtr node, const char *name);
int xmlGetPropAsInt(xmlNodePtr node, const char *name);
int xmlGetPropAsEnum(
    xmlNodePtr node, const char *name, const char *enumValues[]
);
int xmlPropCmp(xmlNodePtr node, const char *name, const char *s);
int xmlPropCaseCmp(xmlNodePtr node, const char *name, const char *s);

static inline const xmlChar *c2xc(const char *s)
{
    return reinterpret_cast<const xmlChar *>(s);
}

static inline const char *xc2c(const xmlChar *s)
{
    return reinterpret_cast<const char *>(s);
}

#endif /* XML_H */
