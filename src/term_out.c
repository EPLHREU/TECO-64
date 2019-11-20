///
///  @file    term_out.c
///  @brief   System-independent functions to output to user's terminal.
///
///  @author  Nowwith Treble Software
///
///  @bug     No known bugs.
///
///  @copyright  tbd
///
///  Permission is hereby granted, free of charge, to any person obtaining a copy
///  of this software and associated documentation files (the "Software"), to deal
///  in the Software without restriction, including without limitation the rights
///  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
///  copies of the Software, and to permit persons to whom the Software is
///  furnished to do so, subject to the following conditions:
///
///  The above copyright notice and this permission notice shall be included in
///  all copies or substantial portions of the Software.
///
///  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
///  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
///
////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "teco.h"
#include "ascii.h"
#include "eflags.h"
#include "errors.h"


///
///  @brief    Echo character in a printable form, either as c, ^c, or [c].
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void echo_chr(int c)
{
    if (c == ESC)
    {
        if (f.et.accent)
        {
            c = '`';
        }
        else if (f.ee != NUL)
        {
            c = f.ee;
        }
    }

    if (isprint(c))
    {
        putc_term(c);
    }
    else if (!isascii(c))               // 8-bit character?
    {
        if (f.et.eightbit)              // Can terminal display it?
        {
            putc_term(c);               // Yes
        }
        else                            // No, make it printable
        {
            char chrbuf[5];             // [xx] + NUL

            uint nbytes = snprintf(chrbuf, sizeof(chrbuf), "[%02x]", c);

            assert(nbytes < sizeof(chrbuf));
        }
    }
    else                                // Must be a control character
    {
        switch (c)
        {
            case BS:                    // TODO: is this correct?
            case TAB:
            case LF:
            case CR:
                putc_term(c);

                break;

            case DEL:
                break;

            case ESC:
                putc_term('$');

                break;

            case FF:
                putc_term('\r');
                //lint -fallthrough

            case VT:
                putc_term('\n');
                putc_term('\n');
                putc_term('\n');
                putc_term('\n');

                break;

            case CTRL_G:
                putc_term(CTRL_G);

                //lint -fallthrough

            default:                    // Display as +^c
                putc_term('^');
                putc_term(c + 'A' - 1);

                break;
         }
    }
}


///
///  @brief    Process HELP command (TBD).
///
///  @returns  Returns true if we have a HELP command, else false.
///
////////////////////////////////////////////////////////////////////////////////

bool help_command(void)
{
    bool match = match_cmd("HELP");

    if (!match)
    {
        return false;
    }

    f.ei.lf = true;                     // Discard next chr. if LF

    putc_term(CRLF);

    print_err(E_NYI);                   // TODO: temporary!
}


///
///  @brief    Print detailed information about a bad escape sequence.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void print_badseq(void)
{
    static const char *badseq[] =
    {
        "",
        "",
        "Invalid escape sequence.",
        "The 16384 bit of the ET flag is set, which means",
        "that you are in VT200 mode. In this mode, the",
        "escape character is not used to terminate commands.",
        "It is used to introduce escape sequences. This",
        "allows the function keys to take on meanings. The",
        "accent grave (~) character is the command terminator.",
        "If you want to turn off VT200 mode, say 16384,0ET``",
        "Note that the recognition of accent grave as a",
        "command terminator is controlled by the 8192 bit",
        "of the ET flag, separate from the VT200 bit.",
        "There may be a part of the unrecognized escape",
        "sequence in the command string.  The last line of",
        "the command string is shown to help you recover.",
        "",
        NULL
    };


    for (const char **line = badseq; *line != NULL; ++line)
    {
        print_term(*line);
    }

    store_cmd(SPACE);
}


///
///  @brief    Print the command we just parsed.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void print_cmd(struct cmd *cmd)
{
    // Here when we've parsed the entire command - now type it out.

    printf("command: ");
    fflush(stdout);

    if (cmd->expr.len != 0)
    {
        const char *p = cmd->expr.buf;

        putc_term('{');

        while (p < cmd->expr.buf + cmd->expr.len)
        {
            echo_chr(*p++);
        }

        putc_term('}');
        putc_term(SPACE);
    }

    if (cmd->c1 == ESC)
    {
        putc_term(CRLF);

        return;
    }

    if (cmd->got_colon || cmd->got_dcolon)
    {
        putc_term('{');

        echo_chr(':');

        if (cmd->got_dcolon)
        {
            echo_chr(':');
        }        

        putc_term('}');
        putc_term(SPACE);
    }

    if (cmd->got_atsign)
    {
        putc_term('{');

        echo_chr('@');

        putc_term('}');
        putc_term(SPACE);
    }

    putc_term('{');

    echo_chr(cmd->c1);

    if (cmd->c2 != NUL)
    {
        echo_chr(cmd->c2);

        if (cmd->c3 != NUL)
        {
            echo_chr(cmd->c3);
        }
    }

    putc_term('}');

    putc_term(SPACE);

    if (cmd->qreg)                      // Do we have a Q-register name?
    {
        putc_term('{');

        if (cmd->qlocal)                // Yes, is it local?
        {
            echo_chr('.');
        }

        echo_chr(cmd->qreg);

        putc_term('}');
        putc_term(SPACE);
    }

    if (cmd->text1.len != 0)
    {
        if (cmd->got_atsign)            // Conditionally echo delimiter before 1st arg.
        {
            echo_chr(cmd->delim);
            putc_term(SPACE);
        }

        putc_term('{');

        const char *p = cmd->text1.buf;

        while (p < cmd->text1.buf + cmd->text1.len)
        {
            echo_chr(*p++);
        }

        putc_term('}');
        putc_term(SPACE);
    }

    if (cmd->text2.len != 0 || cmd->delim != ESC)
    {
        echo_chr(cmd->delim);           // Echo delimiter between texts
        putc_term(SPACE);
    }

    if (cmd->text2.len != 0)
    {
        putc_term('{');

        const char *p = cmd->text2.buf;

        while (p < cmd->text2.buf + cmd->text2.len)
        {
            echo_chr(*p++);
        }

        putc_term('}');
        putc_term(SPACE);

        if (cmd->delim != ESC)
        {
            echo_chr(cmd->delim);       // Echo delimiter at end
        }
    }

    putc_term(CRLF);
}
