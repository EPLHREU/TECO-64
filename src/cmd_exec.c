///
///  @file    cmd_exec.c
///  @brief   Execute command string.
///
///  @copyright 2019-2020 Franklin P. Johnston / Nowwith Treble Software
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
#include <ctype.h>
#include <string.h>

#include "teco.h"
#include "ascii.h"
#include "eflags.h"
#include "errcodes.h"
#include "estack.h"
#include "exec.h"
#include "qreg.h"
#include "term.h"

// This prototype is here to avoid errors defining the command tables.

static void exec_escape(struct cmd *unused1);

#include "commands.h"


#if     defined(TECO_DISPLAY)
#define STATIC                      ///< null_cmd has global scope
#else
#define STATIC static               ///< null_cmd has local scope
#endif

uint nparens;                       ///< Parenthesis nesting count

///  @var    null_cmd
///
///  @brief  Initial command block values.

STATIC const struct cmd null_cmd =
{
    .c1     = NUL,
    .c2     = NUL,
    .c3     = NUL,
    .qname  = NUL,
    .qlocal = false,
    .m_set  = false,
    .m_arg  = 0,
    .n_set  = false,
    .n_arg  = 0,
    .h      = false,
    .ctrl_y = false,
    .w      = false,
    .colon  = false,
    .dcolon = false,
    .atsign = false,
    .delim  = ESC,
    .text1  = { .data = NULL, .len = 0 },
    .text2  = { .data = NULL, .len = 0 },
};


// Local functions

static void check_qreg(const struct cmd_table *entry, struct cmd *cmd);

static void end_cmd(struct cmd *cmd, enum cmd_opts opts);

static exec_func *next_cmd(struct cmd *cmd);

static const struct cmd_table *scan_cmd(struct cmd *cmd, int c);

static const struct cmd_table *scan_ef(struct cmd *cmd,
                                       const struct cmd_table *table,
                                       uint count, int error);

static void scan_text(int delim, struct tstring *text);

static void scan_texts(struct cmd *cmd, enum cmd_opts opts);


///
///  @brief    Check to see if we've already processed H or CTRL/Y.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void check_args(struct cmd *cmd)
{
    assert(cmd != NULL);                // Error if no command block

    if (f.e2.args && (cmd->h || cmd->ctrl_y))
    {
        throw(E_ARG);                   // Improper arguments
    }
}


///
///  @brief    Check to see if command requires a Q-register.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void check_qreg(const struct cmd_table *entry, struct cmd *cmd)
{
    assert(entry != NULL);
    assert(cmd != NULL);

    int c = fetch_cbuf();               // Get Q-register (or dot)

    if (c == '.')                       // Is it local?
    {
        cmd->qlocal = true;             // Yes, mark it

        c = fetch_cbuf();               // Get Q-register name for real
    }

    // Q-registers must be alphanumeric. G commands also allow *, _, and +.

    if (!isalnum(c))
    {
        if (toupper(cmd->c1) != 'G' || strchr("*_+", c) == NULL)
        {
            throw(E_IQN, c);            // Invalid Q-register name
        }
    }

    cmd->qname = (char)c;               // Save the name
}


///
///  @brief    Check numeric arguments.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void end_cmd(struct cmd *cmd, enum cmd_opts opts)
{
    assert(cmd != NULL);                // Error if no command block

    // See if we have an n argument. If not, then check to see if the command
    // was preceded by a minus sign, which is equivalent to an argument of -1.

    cmd->n_set = pop_expr(&cmd->n_arg);

    if (cmd->n_set == false && unary_expr())
    {
        cmd->n_set = true;
        cmd->n_arg = -1;
    }

    // See if command consumes numeric arguments.

    if (opts & OPT_E)
    {
        cmd->m_set = cmd->n_set = false;
        cmd->m_arg = cmd->n_arg = 0;
    }

    // If we have an m argument, verify that it is valid for this command, and
    // that it is followed by an n argument.

    if (cmd->m_set)
    {
        if (f.e2.m_arg && !(opts & OPT_M))
        {
            throw(E_IMA);               // Invalid m argument
        }
        else if (!cmd->n_set)
        {
            throw(E_NON);               // No n argument after m argument
        }
    }
    else if (cmd->n_set)
    {
        if (f.e2.n_arg && !(opts & OPT_N))
        {
            throw(E_INA);               // Invalid n argument
        }
    }
}


///
///  @brief    Execute command string.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_cmd(struct cmd *macro)
{
    struct cmd cmd = null_cmd;

    // If we were called from a macro, then copy any numeric arguments.

    if (macro != NULL)
    {
        if (macro->n_set)
        {
            push_expr(macro->n_arg, EXPR_VALUE);
        }

        cmd.m_set = macro->m_set;
        cmd.m_arg = macro->m_arg;
    }

    // Loop for all commands in command string.

    while (cbuf->len != 0)
    {
        exec_func *exec = next_cmd(&cmd);

        if (exec == NULL)
        {
            break;
        }

        int c = cmd.c1;

        if (cmd.m_set && cmd.m_arg < 0 && toupper(c) != 'W')
        {
            throw(E_NCA);               // Negative argument to comma
        }

        (*exec)(&cmd);

        cmd = null_cmd;

        // Check for commands that allow numeric arguments to be passed
        // through to subsequent commands. This includes [ and ], but
        // also !, because it's sometimes useful to interpose comments
        // between two commands.

        if (strchr("![]", c) != NULL)
        {
            cmd.n_set = pop_expr(&cmd.n_arg);
            cmd.m_set = pop_expr(&cmd.m_arg);

            if (cmd.n_set)
            {
                push_expr(cmd.n_arg, EXPR_VALUE);
            }
        }

        if (f.e0.ctrl_c)                // If CTRL/C typed, return to main loop
        {
            f.e0.ctrl_c = false;

            throw(E_XAB);               // Execution aborted
        }
    }
}


///
///  @brief    Execute ESCape command. We're called here only for ESCapes
///            between commands, or at the end of command strings, not for
///            ESCapes used to delimit text arguments after commands.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void exec_escape(struct cmd *unused1)
{
    // Skip past any whitespace after the ESCape.

    while (!empty_cbuf())
    {
        int c = peek_cbuf();

        if (!isspace(c) || c == TAB)    // Whitespace?
        {
            break;                      // No, so we're done skipping chrs.
        }

        (void)fetch_cbuf();
    }

    // If we've read all characters in command string, then reset for next time.

    if (empty_cbuf())
    {
        cbuf->pos = cbuf->len = 0;
    }
}


///
///  @brief    Scan command string for next command. Since many commands are
///            used only to create expressions (such as numeric arguments) for
///            other commands, we will continue looping here until we have a
///            complete command.
///
///  @returns  Command function to execute, or NULL if at end of command string.
///
////////////////////////////////////////////////////////////////////////////////

static exec_func *next_cmd(struct cmd *cmd)
{
    assert(cmd != NULL);                // Error if no command block

    while (!empty_cbuf())
    {
        int c = fetch_cbuf();

        if (c == SPACE || c == LF || c == VT || c == FF || c == CR)
        {
            continue;                   // Just skip whitespace
        }

        const struct cmd_table *entry = scan_cmd(cmd, c);

        if (entry == NULL)
        {
            continue;
        }

        enum cmd_opts opts = entry->opts;

        if ((cmd->atsign && f.e2.atsign && !(opts & OPT_A)))
        {
            throw(E_ATS);               // Invalid at-sign
        }

        if ((cmd->colon  && f.e2.colon  && !(opts & OPT_C)) ||
            (cmd->dcolon && f.e2.colon  && !(opts & OPT_D)))
        {
            throw(E_COL);               // Invalid colon
        }

        if (opts & OPT_Q)               // Check for any required Q-register
        {
            check_qreg(entry, cmd);
        }

        if (opts & OPT_T1)              // Check for optional text arguments
        {
            scan_texts(cmd, opts);
        }

        // Handle commands that require special cases. These include A, which
        // is a operand command if it's preceded by an expression, but not a
        // colon, ^Q, which is a operand if preceded by an expression, and
        // 'flag' commands (e.g, ET), which are operands if they are NOT
        // preceded by an expression.

        if (c == 'A' || c == 'a')
        {
            if (check_expr() && !cmd->colon)
            {
                end_cmd(cmd, opts);     // Do final check

                opts |= OPT_O;
            }
        }
        else if (c == CTRL_Q)
        {
            if (check_expr())
            {
                end_cmd(cmd, opts);     // Do final check
            }

            opts |= OPT_O;
        }
        else if ((opts & OPT_F) && !check_expr())
        {
            opts |= OPT_O;
        }

        if (!(opts & OPT_O))            // Operator or operand?
        {
            end_cmd(cmd, opts);         // Do final check

            return entry->exec;         // Tell caller to execute
        }

        (*entry->exec)(cmd);            // Execute and continue

        // If command allowed @, ensure we don't pass it to next command

        if (opts & OPT_A)
        {
            cmd->atsign = false;
        }

        // If command allowed :, ensure we don't pass it to next command

        if (opts & OPT_C)
        {
            cmd->colon = cmd->dcolon = false;
        }
    }

    // If we're not in a macro, then confirm that parentheses were properly
    // matched, and that there's nothing left on the expression stack.

    if (!check_macro())
    {
        // Here if we've reached the end of the command string.

        if (nparens)
        {
            throw(E_MRP);               // Missing right parenthesis
        }

        if (f.e2.args && estack.base != estack.level)
        {
            throw(E_ARG);               // Improper arguments
        }
    }

    return NULL;
}


///
///  @brief    Find table entry for command.
///
///  @returns  Table entry for function, or NULL if nothing to do.
///
////////////////////////////////////////////////////////////////////////////////

static const struct cmd_table *scan_cmd(struct cmd *cmd, int c)
{
    assert(cmd != NULL);                // Error if no command block

    // Reset the fields that can change from command to command.

    cmd->c1     = (char)c;
    cmd->c2     = NUL;
    cmd->c3     = NUL;
    cmd->qname  = NUL;
    cmd->qlocal = false;

    if (c < 0 || c >= (int)cmd_max)
    {
        throw(E_ILL, c);                // Illegal command
    }

    const struct cmd_table *entry = &cmd_table[c];

    switch (c)
    {
        case '"':
            cmd->c2 = (char)fetch_cbuf();

            return entry;

        case '=':
            if (!empty_cbuf() && peek_cbuf() == '=')
            {
                (void)fetch_cbuf();

                cmd->c2 = cmd->c1;

                if (!empty_cbuf() && peek_cbuf() == '=')
                {
                    (void)fetch_cbuf();

                    cmd->c3 = cmd->c1;
                }
            }

            return entry;

        case ':':
            if (peek_cbuf() == ':')     // Double colon?
            {
                (void)fetch_cbuf();     // Yes, count it

                if (cmd->dcolon && f.e2.colon)
                {
                    throw(E_COL);       // Too many colons
                }

                cmd->dcolon = true;     // And flag it
            }

            cmd->colon = true;          // Flag the first colon

            return NULL;

        case '@':
            if (cmd->atsign && f.e2.atsign)
            {
                throw(E_ATS);           // Too many at signs
            }

            cmd->atsign = true;

            return NULL;

        case 'E':
        case 'e':
            entry = scan_ef(cmd, e_table, e_max, E_IEC);

            break;

        case 'F':
        case 'f':
            entry = scan_ef(cmd, f_table, f_max, E_IFC);

            break;

        case 'P':
        case 'p':
            if (!empty_cbuf() && toupper(peek_cbuf()) == 'W')
            {
                (void)fetch_cbuf();

                cmd->w = true;
            }

            break;

        case '^':
        case '\x1E':
            check_args(cmd);

            if (c == '\x1E' || (c = fetch_cbuf()) == '^')
            {
                c = fetch_cbuf();

                push_expr((int_t)c, EXPR_VALUE);

                return NULL;
            }

            c = 1 + toupper(c) - 'A';

            if (c <= NUL || c >= SPACE)
            {
                throw(E_IUC, c);        // Invalid character following ^
            }

            cmd->c1 = (char)c;

            entry = &cmd_table[c];

            break;

        default:
            if (nparens != 0 && f.e1.xoper && exec_xoper(c))
            {
                if (c != '{' && c != '}')
                {
                    check_args(cmd);
                }

                return NULL;
            }
    }

    if (entry->exec == NULL)
    {
        throw(E_ILL, c);                // Illegal command
    }

    return entry;
}


///
///  @brief    Scan 2nd character for E or F command.
///
///  @returns  Table entry.
///
////////////////////////////////////////////////////////////////////////////////

static const struct cmd_table *scan_ef(struct cmd *cmd,
                                       const struct cmd_table *table,
                                       uint count, int error)
{
    assert(cmd != NULL);                // Error if no command block
    assert(table != NULL);              // Error if no command table pointer

    int c = fetch_cbuf();

    if (c < 0 || (uint)c > count || table[c].exec == NULL)
    {
        throw(error, c);                // Invalid E or F character
    }

    cmd->c2 = (char)c;

    return &table[c];
}


///
///  @brief    Scan the text string following the command.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void scan_text(int delim, struct tstring *text)
{
    assert(text != NULL);

    text->data = cbuf->data + cbuf->pos;

    uint n = cbuf->len - cbuf->pos;
    char *end = memchr(text->data, delim, (ulong)n);

    if (end == NULL)
    {
        if (check_macro())
        {
            throw(E_UTM);               // Unterminated macro
        }
        else
        {
            throw(E_UTC);               // Unterminated command
        }
    }

    text->len = (uint)(end - text->data);

    cbuf->pos += text->len + 1;
}


///
///  @brief    Scan for text strings following command.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void scan_texts(struct cmd *cmd, enum cmd_opts opts)
{
    assert(cmd != NULL);                // Error if no command block

    // Only at-sign form of '=' command only allows text arguments.

    if (cmd->c1 == '=' && !cmd->atsign)
    {
        return;
    }

    // Here to check for text arguments. The standard delimiter is ESCape,
    // except for CTRL/A and ! commands. If an at-sign was specified, then
    // the (non-whitespace) delimiter follows the command.

    if (cmd->c1 == CTRL_A)
    {
        cmd->delim = CTRL_A;            // ^A normally ends with ^A
    }
    else if (cmd->c1 == '!')
    {
        // If feature enabled, !! starts a comment that ends with LF.

        if (f.e1.bang && !empty_cbuf() && peek_cbuf() == '!')
        {
            (void)fetch_cbuf();

            cmd->delim = LF;            // Tag goes to end of line
        }
        else
        {
            cmd->delim = '!';           // ! normally ends with !
        }
    }
    else
    {
        cmd->delim = ESC;               // Standard delimiter
    }

    // If the user specified the at-sign modifier, then skip any whitespace
    // between the command and the delimiter.

    if (cmd->atsign)                    // @ modifier?
    {
        while (!empty_cbuf() && peek_cbuf() == ' ')
        {
            (void)fetch_cbuf();
        }

        // Treat first non-whitespace character as text delimiter.

        cmd->delim = (char)fetch_cbuf();

        if (!isgraph(cmd->delim))
        {
            throw(E_ATS);               // Invalid delimiter
        }
    }

    if (cmd->delim != '{' || !f.e1.text)
    {
        scan_text(cmd->delim, &cmd->text1);

        if (opts & OPT_T2)
        {
            scan_text(cmd->delim, &cmd->text2);
        }

        return;
    }

    // Here if user wants to delimit text string(s) with braces. This means
    // the text strings may be of the form {xxx}, and may include whitespace.
    // This allows for commands such as @S {foo} or @FS {foo} {baz}.

    scan_text('}', &cmd->text1);

    if (!(opts & OPT_T2))
    {
        return;
    }

    // Skip any whitespace after '}'

    while (!empty_cbuf() && isspace(peek_cbuf()))
    {
        (void)fetch_cbuf();
    }

    int c = fetch_cbuf();

    scan_text(c == '{' ? '}' : c, &cmd->text2);
}


///
///  @brief    Scan command string for next command. Since many commands are
///            used only to create expressions (such as numeric arguments) for
///            other commands, we will continue looping here until we have a
///            complete command.
///
///            The skip parameter determines whether we just ignore commands
///            until we find one that matches one of the characters in the
///            string, at which time we return to the caller. This is used for
///            branch and loop commands such as ", F>, and O.
///
///  @returns  true if found another command, false if at end of command string.
///
////////////////////////////////////////////////////////////////////////////////

bool skip_cmd(struct cmd *cmd, const char *skip)
{
    assert(cmd != NULL);
    assert(skip != NULL);

    *cmd = null_cmd;

    // Some of the commands we parse may push operands or operators on the
    // stack. Since we don't need those when we find the command we want,
    // we just reset the expression stack when we return. If an error occurs,
    // the entire stack will be reset elsewhere.

    uint saved_level = estack.level;

    while (!empty_cbuf())
    {
        int c = fetch_cbuf();

        if (c == SPACE || c == LF || c == VT || c == FF || c == CR)
        {
            continue;                   // Just skip whitespace
        }

        const struct cmd_table *entry = scan_cmd(cmd, c);

        if (entry == NULL)
        {
            continue;
        }

        enum cmd_opts opts = entry->opts;

        if (opts & OPT_Q)               // Check for any required Q-register
        {
            check_qreg(entry, cmd);
        }

        if (opts & OPT_T1)              // Check for optional text arguments
        {
            scan_texts(cmd, opts);
        }

        if (strchr(skip, cmd->c1) != NULL)
        {
            estack.level = saved_level;

            return true;                // Found what we were looking for
        }

        if (!(opts & OPT_O))            // If not operator or operand,
        {
            *cmd = null_cmd;            //  then reset
        }
    }

    estack.level = saved_level;

    return false;
}
