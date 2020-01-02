///
///  @file    radix_cmd.c
///  @brief   Execute radix commands.
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
#include <stdio.h>
#include <stdlib.h>

#include "teco.h"
#include "errors.h"
#include "exec.h"


///
///  @brief    Execute ^D (CTRL/D) command: switch radix to decimal.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_ctrl_d(struct cmd *cmd)
{
    assert(cmd != NULL);

    if (!scan.dryrun)
    {
        v.radix = 10;                   // Set radix to decimal
    }
}


///
///  @brief    Execute ^O (CTRL/O) command: switch radix to octal.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_ctrl_o(struct cmd *cmd)
{
    assert(cmd != NULL);

    if (!scan.dryrun)
    {
        v.radix = 8;                    // Set radix to octal
    }
}


///
///  @brief    Scan ^R (CTRL/R) command: read current radix.
///
///  @returns  nothing.
///
////////////////////////////////////////////////////////////////////////////////

void scan_ctrl_r(struct cmd *cmd)
{
    assert(cmd != NULL);

    int n;

    if (scan.dryrun)
    {
        return;
    }

    if (pop_expr(&n))                   // n^R?
    {
        if (n != 8 && n != 10 && n != 16)
        {
            print_err(E_IRA);           // Illegal radix argument
        }

        v.radix = n;                    // Set the radix
    }
    else
    {
        push_expr(v.radix, EXPR_VALUE); // No, just save what we have
    }
}
