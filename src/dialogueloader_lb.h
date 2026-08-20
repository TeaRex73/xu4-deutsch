/*
 * $Id$
 */

#ifndef DIALOGUELOADER_LB_H
#define DIALOGUELOADER_LB_H

#include "dialogueloader.h"


/**
 * The dialogue loader for Lord British
 */
class U4LBDialogueLoader:public DialogueLoader {
public:
    Dialogue *load(void *source) override;

 private:
    static DialogueLoader *instance;
};

#endif
