///
///  @file    goto_cmd.c
///  @brief   Execute goto commands.
///
///  @bug     No known bugs.
///
///  @copyright  2019-2020 Franklin P. Johnston
///
///  Permission is hereby granted, free of charge, to any person obtaining a
///  copy of this software and associated documentation files (the "Software"),
///  to deal in the Software without restriction, including without limitation
///  the rights to use, copy, modify, merge, publish, distribute, sublicense,
///  and/or sell copies of the Software, and to permit persons to whom the
///  Software is furnished to do so, subject to the following conditions:
///
///  The above copyright notice and this permission notice shall be included in
///  all copies or substantial portions of the Software.
///
///  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIA-
///  BILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
///  THE SOFTWARE.
///
////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "teco.h"
#include "eflags.h"
#include "errors.h"
#include "estack.h"
#include "exec.h"


// Local functions

static void find_tag(struct cmd *cmd, const char *text, uint len);


///
///  @brief    Execute ! command: comment/tag. This function doesn't actually do
///            anything, but it exists to ensure that the command is properly
///            scanned, so that tags can be found with the O and nO commands.
///
///            Note that we pass through any numeric arguments, much like the [
///            and ] commands. This allows comments to be inserted between two
///            commands, the second of which uses the arguments resulting from
///            the first, such as the following:
///
///            @{ :@ER/foo/                ! Open input file ! @}
///               "U :@^A/?No file/ EX '   ! Print message and exit if error !
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_bang(struct cmd *cmd)
{
    assert(cmd != NULL);

    if (cmd->n_set)                     // Pass through m and n arguments
    {
        if (cmd->m_set)
        {
            push_expr(cmd->m_arg, EXPR_VALUE);
        }

        push_expr(cmd->n_arg, EXPR_VALUE);
    }
}


///
///  @brief    Execute O command: goto and computed goto.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_O(struct cmd *cmd)
{
    assert(cmd != NULL);

    if (cmd->text1.len == 0)            // Is there a tag?
    {
        print_err(E_NOT);               // O command has no tag
    }

    // Here if we have a tag

    if (!cmd->n_set)                    // Is it Otag` or nOtag1,tag,tag3`?
    {
        find_tag(cmd, cmd->text1.buf, cmd->text1.len);

        return;
    }

    // Here if the command was nO (computed goto).

    if (cmd->n_arg <= 0)                // Is value non-positive?
    {
        print_err(E_NOA);               // O argument is <= 0
    }

    // Parse the comma-separated list of tags, looking for the one we want.
    // If the nth tag is null, or is out of range, then we do nothing.

    char taglist[cmd->text1.len + 1];
    char *buf = taglist;
    char *saveptr;
    char *next;
    uint ntags = 0;

    sprintf(taglist, "%.*s", (int)cmd->text1.len, cmd->text1.buf);

    // Find all tags, looking for the one matching the n argument.

    while ((next = strtok_r(buf, ",", &saveptr)) != NULL)
    {
        buf = NULL;

        if (++ntags == (uint)cmd->n_arg) // Is this the tag we want?
        {
            uint len = (uint)strlen(next);

            if (len != 0)
            {
                find_tag(cmd, next, len);
            }

            return;
        }
    }

    // Here if O argument is out of range of tag list

    print_err(E_BOA);                   // O argument is out of range
}


///
///  @brief    Find a specific tag, checking for possible duplicates.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void find_tag(struct cmd *cmd, const char *text, uint len)
{
    assert(cmd != NULL);
    assert(text != NULL);

    int tag_pos = -1;                   // Position of tag in command string

    // The following creates a tag using string building characters, but then
    // sets up a local copy so that we can free up the string that was allocated
    // by build_string(). This is to avoid memory leaks.

    char *tag1 = NULL;                  // Dynamically-allocated tag name

    (void)build_string(&tag1, text, len);

    len = (uint)strlen(tag1);

    char tag2[len + 1];                 // Local copy of tag name

    strcpy(tag2, tag1);

    free_mem(&tag1);

    current->pos = 0;                   // Start at beginning of command

    // Scan entire command string to verify that we have
    // one and only one instance of the specified tag.

    while (current->pos < current->len)
    {
        bool dryrun = f.e0.dryrun;

        f.e0.dryrun = true;

        (void)next_cmd(cmd);            // Get next command

        f.e0.dryrun = dryrun;

        if (cmd->c1 != '!')             // Is this a tag?
        {
            continue;                   // No
        }

        if (cmd->text1.len == len &&
            !memcmp(cmd->text1.buf, tag2, (ulong)len))
        {
            if (tag_pos != -1)          // Found tag. Have we seen it already?
            {
                prints_err(E_DUP, tag2); // Duplicate tag
            }

            tag_pos = (int)current->pos; // Remember tag for later
        }
    }

    if (tag_pos == -1)                  // Did we find the tag?
    {
        prints_err(E_TAG, tag2);        // Missing tag
    }

    current->pos = (uint)tag_pos;       // Execute goto
}
