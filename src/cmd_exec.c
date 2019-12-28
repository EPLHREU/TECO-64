///
///  @file    cmd_exec.c
///  @brief   Execute command string.
///
///  @bug     No known bugs.
///
///  @copyright  2019-2020 Franklin P. Johnston
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
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "teco.h"
#include "eflags.h"
#include "errors.h"
#include "exec.h"


enum scan_state scan_state;             ///< Current expression scanning state

// Local functions

static void finish_cmd(struct cmd *cmd, exec_func *exec);


///
///  @brief    Execute command string.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_cmd(void)
{
    struct cmd cmd = null_cmd;

    scan_state = SCAN_PASS1;

    // Loop for all characters in command string.

    for (;;)
    {
        // The following is used to note the start of any expression that may
        // precede the actual command. Until we start one, we will keep setting
        // a pointer to our current location in the command string. Once we
        // start an expression, we will increment the count for each character
        // parsed. Once we're done with the expression, we will have a pointer
        // to the start of it, along with the length of the no. of characters.
        //
        // Note that one of the reasons we do it this way is so that we will
        // count will be correct even if the expression contains characters
        // such as spaces which don't affect its evaluation, but which need to
        // be counted anyway.

        if (cmd.expr.len == 0)
        {
            cmd.expr.buf = next_buf();
        }

        if (next_buf() == NULL)         // If end of command string,
        {
            break;                      //  then back to main loop
        }

        int c = fetch_buf();            // Get next command string character

        // Flag that command execution has started. This is placed after we
        // call fetch_buf(), so that any CTRL/C causes return to main loop
        // without an error message.

        // TODO: add check for trace mode

        f.ei.exec = true;

        if ((uint)c >= cmd_count)
        {
            printc_err(E_ILL, c);       // Illegal character
        }

        cmd.c1 = (char)c;

        exec_func *exec = scan_pass1(&cmd);

        if (scan_state == SCAN_PASS1)    // Still scanning expression?
        {
            // If we haven't started scanning an expression, and we just read a
            // whitespace character, then just skip it. Note that if we see a
            // TAB, we'll switch state to SCAN_PASS2, so there's no concern
            // that the code below will cause TABs to be ignored.

            if (cmd.expr.len == 0 || !isspace(c))
            {
                cmd.expr.len = (uint)(next_buf() - cmd.expr.buf);
            }
        }
        else if (scan_state == SCAN_PASS2) // Done scanning expression?
        {
            finish_cmd(&cmd, exec);

            scan_state = SCAN_PASS1;
        }

        f.ei.exec = false;              // Suspend this to check CTRL/C

        if (f.ei.ctrl_c)                // If CTRL/C typed, return to main loop
        {
            f.ei.ctrl_c = false;

            print_err(E_XAB);
        }
    }
}


///
///  @brief    Process ESCape which terminates a command string.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_escape(struct cmd *cmd)
{
    assert(cmd != NULL);

    init_expr();
}


///
///  @brief    Finish scanning command, after we have done a preliminary scan
///            for an expression, and for any modifiers.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void finish_cmd(struct cmd *cmd, exec_func *exec)
{
    assert(cmd != NULL);

    scan_tail(cmd);                     // Finish scanning command

    // Tags that start with !+ are really comments, so discard them.

    if (cmd->c1 == '!')
    {
        if (cmd->text1.len > 0)
        {
            if (cmd->text1.buf[0] == '+')
            {
                return;
            }
        }
    }

    if (cmd->c1 == '\'' || cmd->c1 == '|' || cmd->c1 == '>')
    {
        --cmd->level;
    }

    log_cmd(cmd);

    if (cmd->c1 == '"' || cmd->c1 == '|' || cmd->c1 == '<')
    {
        ++cmd->level;
    }

    if (cmd->expr.len != 0)             // Do we have an expression?
    {
        init_expr();                    // Re-initialize the stack

        // Save the stuff we'll need for the actual command

        char c1 = cmd->c1;
        char c2 = cmd->c2;
        char c3 = cmd->c3;
        uint atsign_set = cmd->atsign_set;
        uint colon_set  = cmd->colon_set;
        uint dcolon_set = cmd->dcolon_set;

        scan_pass2(cmd);

        // Restore command characters

        cmd->dcolon_set = dcolon_set;
        cmd->colon_set  = colon_set;
        cmd->atsign_set = atsign_set;
        cmd->c3 = c3;
        cmd->c2 = c2;
        cmd->c1 = c1;
    }

    // If the only thing on the expression stack is a minus sign, then say we
    // have an n argument equal to -1.
    //
    // If we have anything else on the stack after parsing an expression, then
    // that's an error.

    if (cmd->expr.len == 1 && estack.obj[0].type == EXPR_MINUS)
    {
        estack.level = 0;

        cmd->n_set = true;
        cmd->n_arg = -1;
    }
    else if (pop_expr(&cmd->n_arg))
    {
        cmd->n_set = true;

        if (pop_expr(&cmd->m_arg))
        {
            cmd->m_set = true;
        }
    }
    else if (estack.level != 0)
    {
        print_err(E_ARG);
    }

    if (exec != NULL)
    {
        if (!teco_debug || cmd->c1 == 'e')
        {
            (*exec)(cmd);
        }
    }

    int level = cmd->level;        

    *cmd = null_cmd;

    cmd->level = level;

    if (pop_expr(&cmd->n_arg))          // Pass through n to next command?
    {
        cmd->n_set = true;
    }

    assert(estack.level == 0);          // Sanity check
}

