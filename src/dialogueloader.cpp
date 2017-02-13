/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstdlib>
#include <cstring>
#include "debug.h"
#include "conversation.h"
#include "dialogueloader.h"

std::map<std::string, DialogueLoader *> *DialogueLoader::loaderMap = nullptr;

DialogueLoader *DialogueLoader::getLoader(const std::string &mimeType)
{
    ASSERT(
        loaderMap != nullptr,
        "DialogueLoader::getLoader loaderMap not initialized"
    );
    if (loaderMap->find(mimeType) == loaderMap->end()) {
        return nullptr;
    }
    return (*loaderMap)[mimeType];
}

DialogueLoader *DialogueLoader::registerLoader(
    DialogueLoader *loader, const std::string &mimeType
)
{
    if (loaderMap == nullptr) {
        loaderMap = new std::map<std::string, DialogueLoader *>;
    }
    (*loaderMap)[mimeType] = loader;
    return loader;
}

void DialogueLoader::cleanup()
{
    for (std::map<std::string, DialogueLoader *>::iterator i =
             loaderMap->begin();
         i != loaderMap->end();
         i++) {
        delete i->second;
    }
    delete loaderMap;
}
