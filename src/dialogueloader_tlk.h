/*
 * $Id$
 */

#ifndef DIALOGUELOADER_TLK_H
#define DIALOGUELOADER_TLK_H

#include "dialogueloader.h"


/**
 * The dialogue loader for u4dos .tlk files
 */
class U4TlkDialogueLoader:public DialogueLoader {
public:
    virtual Dialogue *load(void *source) override;

 private:
    static DialogueLoader *instance;
};

#endif
