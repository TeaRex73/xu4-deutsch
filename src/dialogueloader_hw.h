/*
 * $Id$
 */

#ifndef DIALOGUELOADER_HW_H
#define DIALOGUELOADER_HW_H

#include "dialogueloader.h"


/**
 * The dialogue loader for Hawkwind.
 */
class U4HWDialogueLoader:public DialogueLoader {
public:
    virtual Dialogue *load(void *source) override;

 private:
    static DialogueLoader *instance;
};

#endif
