/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cctype>
#include <cstring>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/valid.h>
#include <libxml/xmlIO.h>
#include <libxml/xinclude.h>
#include <libxml/xpath.h>

#if 0
// we rely on xinclude support
#ifndef LIBXML_XINCLUDE_ENABLED
#error "xinclude not available: libxml2 not compiled with xinclude support"
#endif
#endif

#include "config.h"
#include "error.h"
#include "settings.h"
#include "u4file.h"
#include "xml.h"


extern bool verbose;
Config *Config::instance = nullptr;
char DEFAULT_CONFIG_XML_LOCATION[] = "config.xml";
char *Config::CONFIG_XML_LOCATION_POINTER = &DEFAULT_CONFIG_XML_LOCATION[0];

void Config::destroy()
{
    xmlFreeDoc(getInstance()->doc);
    xmlCleanupParser();
    delete instance;
}

const Config *Config::getInstance()
{
    if (!instance) {
        xmlRegisterInputCallbacks(
            &xmlFileMatch, &fileOpen, xmlFileRead, xmlFileClose
        );
        instance = new Config;
    }
    return instance;
}

ConfigElement Config::getElement(const std::string &name) const
{
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;
    std::string path = "/config/" + name;
    context = xmlXPathNewContext(doc);
    result = xmlXPathEvalExpression(
        c2xc(path.c_str()), context
    );
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        errorFatal("no match for xpath %s\n", path.c_str());
    }
    xmlXPathFreeContext(context);
    if (result->nodesetval->nodeNr > 1) {
        errorWarning(
            "more than one match for xpath %s\n", path.c_str()
        );
    }
    xmlNodePtr node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result);
    return ConfigElement(node);
}

Config::Config()
    :doc(
        xmlReadFile(
        Config::CONFIG_XML_LOCATION_POINTER,
        nullptr,
        XML_PARSE_NOENT | XML_PARSE_XINCLUDE
        )
    )
{
    if (!doc) {
        std::printf(
            "Failed to read core config.xml. Assuming it is located at '%s'",
            Config::CONFIG_XML_LOCATION_POINTER
        );
        errorFatal("error parsing config.xml");
    }
    xmlXIncludeProcess(doc);
    if (settings.validateXml && doc->intSubset) {
        std::string errorMessage;
        xmlValidCtxt cvp = {}; // makes valgrind happy
        if (verbose) {
            std::printf("validating config.xml\n");
        }
        cvp.userData = &errorMessage;
        cvp.error = &accumError;

                // Error changed to not fatal due to regression in libxml2
        if (!xmlValidateDocument(&cvp, doc)) {
            errorWarning("xml parse error:\n%s", errorMessage.c_str());
        }
    }
}

std::vector<std::string> Config::getGames()
{
    std::vector<std::string> result;
    result.push_back("Ultima IV");
    return result;
}

void Config::setGame(const std::string &)
{
}

void *Config::fileOpen(const char *filename)
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

void Config::accumError(void *l, const char *fmt, ...)
{
    std::string *errorMessage = static_cast<std::string *>(l);
    char buffer[1000];
    std::va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    errorMessage->append(buffer);
}

ConfigElement::ConfigElement(xmlNodePtr xmlNode)
    :node(xmlNode), name(xc2c(xmlNode->name))
{
}

ConfigElement::ConfigElement(const ConfigElement &e)
    :node(e.node), name(e.name)
{
}

ConfigElement::~ConfigElement()
{
}

ConfigElement &ConfigElement::operator=(const ConfigElement &e)
{
    if (&e != this) {
        node = e.node;
        name = e.name;
    }
    return *this;
}

bool ConfigElement::exists(const std::string &name) const
{
    xmlChar *prop =
        xmlGetProp(node, c2xc(name.c_str()));
    bool exists = prop != nullptr;
    xmlFree(prop);
    return exists;
}

std::string ConfigElement::getString(const std::string &name) const
{
    xmlChar *prop =
        xmlGetProp(node, c2xc(name.c_str()));
    if (!prop) {
        return "";
    }
    std::string result(xc2c(prop));
    xmlFree(prop);
    return result;
}

int ConfigElement::getInt(const std::string &name, int defaultValue) const
{
    long result;
    xmlChar *prop;
    prop = xmlGetProp(node, c2xc(name.c_str()));
    if (!prop) {
        return defaultValue;
    }
    result = std::strtol(xc2c(prop), nullptr, 0);
    xmlFree(prop);
    return static_cast<int>(result);
}

bool ConfigElement::getBool(const std::string &name) const
{
    int result;
    xmlChar *prop =
        xmlGetProp(node, c2xc(name.c_str()));
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
}

int ConfigElement::getEnum(
    const std::string &name, const char *enumValues[]
) const
{
    int result = -1, i;
    xmlChar *prop;
    prop = xmlGetProp(node, c2xc(name.c_str()));
    if (!prop) {
        return 0;
    }
    for (i = 0; enumValues[i]; i++) {
        if (xmlStrcmp(prop, c2xc(enumValues[i]))
            == 0) {
            result = i;
        }
    }
    if (result == -1) {
        errorFatal("invalid enum value for %s: %s", name.c_str(), prop);
    }
    xmlFree(prop);
    return result;
}

std::vector<ConfigElement> ConfigElement::getChildren() const
{
    std::vector<ConfigElement> result;
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE) {
            result.push_back(ConfigElement(child));
        }
    }
    return result;
}
