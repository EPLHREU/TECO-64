///
///  @file    n_cmd.c
///  @brief   Execute N and FN commands.
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

#include "teco.h"
#include "ascii.h"
#include "eflags.h"
#include "errors.h"
#include "exec.h"
#include "textbuf.h"


// Local functions

static void exec_search(struct cmd *cmd, bool replace);


///
///  @brief    Execute N command: global search.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_N(struct cmd *cmd)
{
    exec_search(cmd, (bool)false);
}


///
///  @brief    Execute FN command: global search and replace.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_FN(struct cmd *cmd)
{
    exec_search(cmd, (bool)true);
}


///
///  @brief    Execute search and replace.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void exec_search(struct cmd *cmd, bool replace)
{
    assert(cmd != NULL);

    if (cmd->n_set && cmd->n_arg == 0)  // 0Ntext` isn't allowed
    {
        print_err(E_ISA);               // Illegal search argument
    }

    if (!cmd->n_set)                    // Ntext` => 1Ntext`
    {
        cmd->n_arg = 1;
        cmd->n_set = true;
    }

    if (cmd->text1.len != 0)
    {
        free_mem(&last_search.buf);

        last_search.len = build_string(&last_search.buf, cmd->text1.buf,
                                       cmd->text1.len);
    }

    struct search s;

    if (cmd->n_arg < 0)
    {
        s.type       = SEARCH_S;
        s.search     = search_backward;
        s.count      = -cmd->n_arg;
        s.text_start = -1;
        s.text_end   = -(int)t.dot;
    }
    else
    {
        s.type       = SEARCH_N;
        s.search     = search_forward;
        s.count      = cmd->n_arg;
        s.text_start = 0;
        s.text_end   = (int)(t.Z - t.dot);
    }

    if (search_loop(&s))
    {
        if (replace)
        {
            delete_tbuf(-(int)last_len);

            if (cmd->text2.len)
            {
                exec_insert(cmd->text2.buf, cmd->text2.len);
            }
        }
        else
        {
            search_print();
        }

        if (cmd->colon_set)
        {
            push_expr(-1, EXPR_VALUE);
        }
    }
    else
    {
        if (cmd->colon_set)
        {
            push_expr(0, EXPR_VALUE);
        }
        else
        {
            if (!f.ed.keepdot)
            {
                setpos_tbuf(0);
            }

            last_search.buf[last_search.len] = NUL;

            prints_err(E_SRH, last_search.buf);
        }
    }
}
