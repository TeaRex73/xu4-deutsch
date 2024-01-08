/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstdlib>
#include <cstdarg>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/valid.h>
#include <libxml/xmlIO.h>

#include "xml.h"
#include "error.h"
#include "settings.h"
#include "u4file.h"


static void xmlAccumError(void *l, const char *fmt, ...);
static void *xmlXu4FileOpen(const char *filename);
static void xmlRegisterIO();
extern bool verbose;
int ioRegistered = 0;

static void *xmlXu4FileOpen(const char *filename)
{
    void *result;
    std::string pathname(u4find_conf(filename));
    if (pathname.empty()) {
        return nullptr;
    }
    result = xmlFileOpen(pathname.c_str());
    if (verbose) {
        std::printf(
            "xml parser opened %s: %s\n",
            pathname.c_str(),
            result ? "success" : "failed"
        );
    }
    return result;
}

static void xmlRegisterIO()
{
    xmlRegisterInputCallbacks(&xmlFileMatch,
                              &xmlXu4FileOpen,
                              xmlFileRead,
                              xmlFileClose);
}


/**
 * Parse an XML document, and optionally validate it.  An error is
 * triggered if the parsing or validation fail.
 */
xmlDocPtr xmlParse(const char *filename)
{
    xmlDocPtr doc;
    if (!ioRegistered) {
        xmlRegisterIO();
    }
    doc = xmlReadFile(
        filename, nullptr, XML_PARSE_NOENT | XML_PARSE_XINCLUDE
    );
    if (!doc) {
        errorFatal("error parsing %s", filename);
    }
    if (settings.validateXml && doc->intSubset) {
        std::string errorMessage;
        xmlValidCtxt cvp;
        if (verbose) {
            std::printf("validating %s\n", filename);
        }
        cvp.userData = &errorMessage;
        cvp.error = &xmlAccumError;
        if (!xmlValidateDocument(&cvp, doc)) {
            errorFatal("xml parse error:\n%s",
                       errorMessage.c_str());
        }
    }
    return doc;
} // xmlParse

static void xmlAccumError(void *l, const char *fmt, ...)
{
    std::string *errorMessage = static_cast<std::string *>(l);
    char buffer[1000];
    std::va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    errorMessage->append(buffer);
}

bool xmlPropExists(xmlNodePtr node, const char *name)
{
    xmlChar *prop = xmlGetProp(node, c2xc(name));
    bool exists = (prop != nullptr);
    if (prop) {
        xmlFree(prop);
    }
    return exists;
}

std::string xmlGetPropAsString(xmlNodePtr node, const char *name)
{
    xmlChar *prop;
    if (settings.validateXml && !xmlHasProp(node, c2xc(name))) {
        return "";
    }
    prop = xmlGetProp(node, c2xc(name));
    if (!prop) {
        return "";
    }
    std::string result(xc2c(prop));
    xmlFree(prop);
    return result;
}


/**
 * Get an XML property and convert it to a boolean value.  The value
 * should be "true" or "false", case sensitive.  If it is neither,
 * false is returned.
 */
bool xmlGetPropAsBool(xmlNodePtr node, const char *name)
{
    int result;
    xmlChar *prop;
    if (settings.validateXml && !xmlHasProp(node, c2xc(name))) {
        return false;
    }
    prop = xmlGetProp(node, c2xc(name));
    if (!prop) {
        return false;
    }
    if (xmlStrcmp(prop, c2xc("true")) == 0) {
        result = true;
    } else {
        result = false;
    }
    xmlFree(prop);
    return result;
} // xmlGetPropAsBool


/**
 * Get an XML property and convert it to an integer value.  Returns
 * zero if the property is not set.
 */
int xmlGetPropAsInt(xmlNodePtr node, const char *name)
{
    long result;
    xmlChar *prop;
    if (settings.validateXml && !xmlHasProp(node, c2xc(name))) {
        return 0;
    }
    prop = xmlGetProp(node, c2xc(name));
    if (!prop) {
        return 0;
    }
    result = std::strtol(xc2c(prop), nullptr, 0);
    xmlFree(prop);
    return static_cast<int>(result);
}

int xmlGetPropAsEnum(
    xmlNodePtr node, const char *name, const char *enumValues[]
)
{
    int result = -1, i;
    xmlChar *prop;
    if (settings.validateXml && !xmlHasProp(node, c2xc(name))) {
        return 0;
    }
    prop = xmlGetProp(node, c2xc(name));
    if (!prop) {
        return 0;
    }
    for (i = 0; enumValues[i]; i++) {
        if (xmlStrcmp(prop, c2xc(enumValues[i])) == 0) {
            result = i;
        }
    }
    if (result == -1) {
        errorFatal("invalid enum value for %s: %s", name, prop);
    }
    xmlFree(prop);
    return result;
} // xmlGetPropAsEnum


/**
 * Compare an XML property to another std::string.  The return value is as
 * strcmp.
 */
int xmlPropCmp(xmlNodePtr node, const char *name, const char *s)
{
    int result;
    xmlChar *prop;
    prop = xmlGetProp(node, c2xc(name));
    result = xmlStrcmp(prop, c2xc(s));
    xmlFree(prop);
    return result;
}


/**
 * Compare an XML property to another std::string, case insensitively.  The
 * return value is as str[case]cmp.
 */
int xmlPropCaseCmp(xmlNodePtr node, const char *name, const char *s)
{
    int result;
    xmlChar *prop;
    prop = xmlGetProp(node, c2xc(name));
    result = xmlStrcasecmp(prop, c2xc(s));
    xmlFree(prop);
    return result;
}
