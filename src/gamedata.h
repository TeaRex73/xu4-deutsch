/*
 * $Id$
 */

#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <string>


/**
 * Add the per-user data directory to the game data search path, so a
 * downloaded ultima4.zip is found no matter where the app runs from.
 */
void addUserDataSearchPath(const std::string &path);

/**
 * Download ultima4.zip from a Gentoo mirror into the user data
 * directory and verify its size and SHA-256 checksum.  Returns true
 * when a verified copy is in place afterwards.  When interactive is
 * true, a dialog explains a failure; otherwise the message goes to
 * stderr.
 */
bool downloadGameData(const std::string &userPath, bool interactive);

/**
 * Decode the documentation shipped as *.pdf.xor, using the game data
 * zip as the XOR key.  The PDFs are written to
 * ~/Documents/Ultima IV Deutsch/ and revealed in Finder once when new
 * files were decoded.  Returns that directory, or an empty string when
 * there was nothing to decode.
 */
std::string decodeDocumentation();

#endif
