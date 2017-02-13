/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include "error.h"

#if defined(_WIN32) || defined(__CYGWIN__)

/*
 * Windows: errors shown in message box
 */
#include <windows.h>

void errorFatal(const char *fmt, ...)
{
    char buffer[1000];
    std::va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    MessageBox(nullptr, buffer, "XU4 Error", MB_OK | MB_ICONERROR);
    std::exit(EXIT_FAILURE);
}

void errorWarning(const char *fmt, ...)
{
    char buffer[1000];
    std::va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    MessageBox(nullptr, buffer, "XU4 Warning", MB_OK | MB_ICONWARNING);
}

#elif defined(MACOSX)
/*
 * MacOS X: errors functios defined in objective-c code elsewhere.
 */

#elif GTK2_DIALOGS
/*
 * Linux/Unix with GTK2: errors shown in message box
 */

#include <gtk/gtk.h>

int need_gtk_init = 1;

void errorFatal(const char *fmt, ...)
{
    char buffer[1000];
    std::va_list args;
    GtkWidget *dialog;
    if (need_gtk_init) {
        gtk_init(nullptr, 0);
        need_gtk_init = 0;
    }
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    dialog = gtk_message_dialog_new(
        nullptr, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "xu4: %s", buffer
    );
    va_end(args);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    std::exit(EXIT_FAILURE);
}

void errorWarning(const char *fmt, ...)
{
    char buffer[1000];
    std::va_list args;
    GtkWidget *dialog;
    if (need_gtk_init) {
        gtk_init(nullptr, 0);
        need_gtk_init = 0;
    }
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    dialog = gtk_message_dialog_new(
        nullptr, 0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "xu4: %s", buffer
    );
    va_end(args);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
}

#else // if defined(_WIN32) || defined(__CYGWIN__)

/*
 * no GUI error functions: errors go to standard error stream
 */
void errorFatal(const char *fmt, ...)
{
    std::va_list args;
    std::fprintf(stderr, "xu4: error: ");
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    va_end(args);
    std::fprintf(stderr, "\n");
    std::exit(EXIT_FAILURE);
}

void errorWarning(const char *fmt, ...)
{
    std::va_list args;
    std::fprintf(stderr, "xu4: warning: ");
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    va_end(args);
    std::fprintf(stderr, "\n");
}

#endif // if defined(_WIN32) || defined(__CYGWIN__)
