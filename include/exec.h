///
///  @file    exec.h
///  @brief   Header file for parsing and executing TECO commands.
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

#if     !defined(_EXEC_H)

#define _EXEC_H

#if     !defined(_TECO_H)

#include "teco.h"

#endif

///  @enum   scan_state
///  @brief  Definitions for scanning expressions.

enum scan_state
{
    SCAN_PASS1,                     ///< First expression scan
    SCAN_MOD,                       ///< Command modifier scan
    SCAN_PASS2                      ///< Second expression scan
};

struct scan
{
    enum scan_state state;          ///< Current scan state
    uint nparens;                   ///< No. of unmatched left parentheses
    uint comma_set  : 1;            ///< Comma seen in expression
    uint colon_opt  : 1;            ///< Colon allowed in command
    uint dcolon_opt : 1;            ///< Double colon allowed in command
    uint atsign_opt : 1;            ///< At sign allowed in command
    uint w_opt      : 1;            ///< W allowed (for P)
    uint t1_opt     : 1;            ///< 1st text string allowed in command
    uint t2_opt     : 1;            ///< 2nd text string allowed in command
    uint q_register : 1;            ///< Q-register required
};

extern struct scan scan;

///  @struct cmd
///  @brief  Command block structure.

struct cmd
{
    char c1;                        ///< 1st command character
    char c2;                        ///< 2nd command character (or NUL)
    char c3;                        ///< 3rd command character (or NUL)
    char qname;                     ///< Q-register name
    bool qlocal;                    ///< If true, Q-register is local
    int m_arg;                      ///< m argument
    int n_arg;                      ///< n argument
    uint m_set      : 1;            ///< m argument found
    uint n_set      : 1;            ///< n argument found
    uint h_set      : 1;            ///< H found
    uint w_set      : 1;            ///< W found
    uint colon_set  : 1;            ///< : found
    uint dcolon_set : 1;            ///< :: found
    uint atsign_set : 1;            ///< @ found
    char delim;                     ///< Delimiter for @ modifier
    struct tstring expr;            ///< Expression string
    struct tstring text1;           ///< 1st text string
    struct tstring text2;           ///< 2nd text string
};

///  @typedef  scan_func
///  @brief   Function to scan command.

typedef void (scan_func)(struct cmd *cmd);

///  @typedef exec_func
///  @brief   Function to execute command.

typedef void (exec_func)(struct cmd *cmd);

///  @struct cmd_table
///  @brief  Format of command tables used to parse and execute commands.

struct cmd_table
{
    scan_func *scan;                ///< Scan function
    exec_func *exec;                ///< Execution function
    const char *opts;               ///< Command modifiers and options
};

extern const struct cmd_table cmd_table[];

extern const struct cmd_table cmd_e_table[];

extern const struct cmd_table cmd_f_table[];

extern const uint cmd_count;

extern const uint cmd_e_count;

extern const uint cmd_f_count;

extern enum scan_state scan_state;

// Functions that assist in parsing commands

extern bool append(bool n_set, int n_arg, bool colon_set);

extern bool append_line(void);

extern void get_flag(struct cmd *cmd, uint flag);

extern bool next_page(uint start, uint end, bool ff, bool yank);

extern uint set_flag(struct cmd *cmd, uint flag);

// Functions that execute commands

extern void exec_A(struct cmd *cmd);

extern void exec_C(struct cmd *cmd);

extern void exec_D(struct cmd *cmd);

extern void exec_G(struct cmd *cmd);

extern void exec_I(struct cmd *cmd);

extern void exec_J(struct cmd *cmd);

extern void exec_K(struct cmd *cmd);

extern void exec_L(struct cmd *cmd);

extern void exec_M(struct cmd *cmd);

extern void exec_N(struct cmd *cmd);

extern void exec_O(struct cmd *cmd);

extern void exec_P(struct cmd *cmd);

extern void exec_R(struct cmd *cmd);

extern void exec_S(struct cmd *cmd);

extern void exec_T(struct cmd *cmd);

extern void exec_U(struct cmd *cmd);

extern void exec_V(struct cmd *cmd);

extern void exec_W(struct cmd *cmd);

extern void exec_X(struct cmd *cmd);

extern void exec_Y(struct cmd *cmd);

extern void exec_apos(struct cmd *cmd);

extern void exec_back(struct cmd *cmd);

extern void exec_bang(struct cmd *cmd);

extern void exec_ctrl_a(struct cmd *cmd);

extern void exec_ctrl_c(struct cmd *cmd);

extern void exec_ctrl_d(struct cmd *cmd);

extern void exec_ctrl_e(struct cmd *cmd);

extern void exec_ctrl_i(struct cmd *cmd);

extern void exec_ctrl_o(struct cmd *cmd);

extern void exec_ctrl_r(struct cmd *cmd);

extern void exec_ctrl_t(struct cmd *cmd);

extern void exec_ctrl_u(struct cmd *cmd);

extern void exec_ctrl_v(struct cmd *cmd);

extern void exec_ctrl_w(struct cmd *cmd);

extern void exec_ctrl_x(struct cmd *cmd);

extern void exec_equals(struct cmd *cmd);

extern void exec_insert(const char *buf, uint len);

extern void exec_lt(struct cmd *cmd);

extern void exec_lbracket(struct cmd *cmd);

extern void exec_question(struct cmd *cmd);

extern void exec_quote(struct cmd *cmd);

extern void exec_gt(struct cmd *cmd);

extern void exec_pct(struct cmd *cmd);

extern void exec_rbracket(struct cmd *cmd);

extern void exec_semi(struct cmd *cmd);

extern void exec_ubar(struct cmd *cmd);

extern void exec_vbar(struct cmd *cmd);

// Functions that handle E commands

extern void exec_EA(struct cmd *cmd);

extern void exec_EB(struct cmd *cmd);

extern void exec_EC(struct cmd *cmd);

extern void exec_ED(struct cmd *cmd);

extern void exec_EE(struct cmd *cmd);

extern void exec_EF(struct cmd *cmd);

extern void exec_EG(struct cmd *cmd);

extern void exec_EH(struct cmd *cmd);

extern void exec_EI(struct cmd *cmd);

extern void exec_EK(struct cmd *cmd);

extern void exec_EL(struct cmd *cmd);

extern void exec_EM(struct cmd *cmd);

extern void exec_EN(struct cmd *cmd);

extern void exec_EP(struct cmd *cmd);

extern void exec_EQ(struct cmd *cmd);

extern void exec_ER(struct cmd *cmd);

extern void exec_ES(struct cmd *cmd);

extern void exec_ET(struct cmd *cmd);

extern void exec_EU(struct cmd *cmd);

extern void exec_EV(struct cmd *cmd);

extern void exec_EW(struct cmd *cmd);

extern void exec_EX(struct cmd *cmd);

extern void exec_EY(struct cmd *cmd);

extern void exec_EZ(struct cmd *cmd);

extern void exec_E_pct(struct cmd *cmd);

extern void exec_E_ubar(struct cmd *cmd);

// Functions that handle F commands

extern void exec_FB(struct cmd *cmd);

extern void exec_FC(struct cmd *cmd);

extern void exec_FD(struct cmd *cmd);

extern void exec_FK(struct cmd *cmd);

extern void exec_FN(struct cmd *cmd);

extern void exec_FR(struct cmd *cmd);

extern void exec_FS(struct cmd *cmd);

extern void exec_F_apos(struct cmd *cmd);

extern void exec_F_lt(struct cmd *cmd);

extern void exec_F_gt(struct cmd *cmd);

extern void exec_F_ubar(struct cmd *cmd);

extern void exec_F_vbar(struct cmd *cmd);

// Functions that parse a command string.

extern void scan_A(struct cmd *cmd);

extern void scan_B(struct cmd *cmd);

extern void scan_bad(struct cmd *cmd);

extern void scan_back(struct cmd *cmd);

extern void scan_comma(struct cmd *cmd);

extern void scan_ctrl_b(struct cmd *cmd);

extern void scan_ctrl_e(struct cmd *cmd);

extern void scan_ctrl_f(struct cmd *cmd);

extern void scan_ctrl_h(struct cmd *cmd);

extern void scan_ctrl_n(struct cmd *cmd);

extern void scan_ctrl_q(struct cmd *cmd);

extern void scan_ctrl_r(struct cmd *cmd);

extern void scan_ctrl_s(struct cmd *cmd);

extern void scan_ctrl_t(struct cmd *cmd);

extern void scan_ctrl_x(struct cmd *cmd);

extern void scan_ctrl_y(struct cmd *cmd);

extern void scan_ctrl_z(struct cmd *cmd);

extern void scan_dot(struct cmd *cmd);

extern void scan_EC(struct cmd *cmd);

extern void scan_ED(struct cmd *cmd);

extern void scan_EE(struct cmd *cmd);

extern void scan_EH(struct cmd *cmd);

extern void scan_EJ(struct cmd *cmd);

extern void scan_EO(struct cmd *cmd);

extern void scan_ES(struct cmd *cmd);

extern void scan_ET(struct cmd *cmd);

extern void scan_EU(struct cmd *cmd);

extern void scan_EV(struct cmd *cmd);

extern void scan_EZ(struct cmd *cmd);

extern void scan_H(struct cmd *cmd);

extern void scan_mod(struct cmd *cmd);

extern void scan_operator(struct cmd *cmd);

extern exec_func *scan_pass1(struct cmd *cmd);

extern void scan_pass2(struct cmd *cmd);

extern void scan_pct(struct cmd *cmd);

extern void scan_Q(struct cmd *cmd);

extern void scan_quote(struct cmd *cmd);

extern void scan_tail(struct cmd *cmd);

extern void scan_W(struct cmd *cmd);

extern void scan_Z(struct cmd *cmd);

// Miscellaneous functions

extern bool check_indirect(void);

extern void close_indirect(void);

extern void exit_EG(void);

extern void init_EG(void);
 
extern void init_loop(void);

extern void log_cmd(struct cmd *cmd);

extern exec_func *next_cmd(struct cmd *cmd);

extern int open_indirect(bool default_type);

extern int open_output(const struct cmd *cmd, uint stream);

extern void reset_if(void);

extern bool read_indirect(void);

extern void reset_loop(void);

extern void reset_scan(enum scan_state state);

extern bool test_if(void);

extern bool test_indirect(void);

#endif  // _EXEC_H
