/*
 * $Id$
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise
#include <cstdlib>
#include <string>
#include <vector>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

class ConfigElement;


/**
 * Singleton class that manages the XML configuration tree.
 */
class Config {
public:
    static void destroy();
    static const Config *getInstance();
    ConfigElement getElement(const std::string &name) const;
    static std::vector<std::string> getGames();
    static void setGame(const std::string &name);
    static char *CONFIG_XML_LOCATION_POINTER;

private:
    Config();
    static void *fileOpen(const char *filename);
    static void accumError(void *l, const char *fmt, ...);
    static Config *instance;
    xmlDocPtr doc;
};


/**
 * A single configuration element in the config tree.  Right now, a
 * thin wrapper around the XML DOM element.
 */
class ConfigElement {
public:
    explicit ConfigElement(xmlNodePtr xmlNode);
    ConfigElement(const ConfigElement &e);
    ~ConfigElement();
    ConfigElement &operator=(const ConfigElement &e);

    const std::string &getName() const
    {
        return name;
    }

    bool exists(const std::string &name) const;
    std::string getString(const std::string &name) const;
    int getInt(const std::string &name, int defaultValue = 0) const;
    bool getBool(const std::string &name) const;
    int getEnum(const std::string &name, const char *enumValues[]) const;
    std::vector<ConfigElement> getChildren() const;

    xmlNodePtr getNode() const
    {
        return node;
    }

private:
    xmlNodePtr node;
    std::string name;
};

#endif /* CONFIG_H */
