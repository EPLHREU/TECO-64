///
///  @file    file.c
///  @brief   File-handling functions.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "teco.h"


struct ifile ifiles[IFILE_MAX];         ///< Input file descriptors

struct ofile ofiles[OFILE_MAX];         ///< Output file descriptors

uint istream = IFILE_PRIMARY;           ///< Current input stream

uint ostream = OFILE_PRIMARY;           ///< Current output stream

char *last_file = NULL;                 ///< Last file opened

char *filename_buf;                     ///< Allocated space for filename

// Local functions

static void file_exit(void);


///
///  @brief    Close out file streams.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void file_exit(void)
{
    dealloc(&filename_buf);

    FILE *fp;

    for (int i = 0; i < IFILE_MAX; ++i)
    {
        if ((fp = ifiles[i].fp) != NULL)
        {
            fclose(fp);
        }

        ifiles[i].fp  = NULL;
        ifiles[i].eof = false;
        ifiles[i].cr  = false;
    }

    istream = IFILE_PRIMARY;

    for (int i = 0; i < OFILE_MAX; ++i)
    {
        if ((fp = ofiles[i].fp) != NULL)
        {
            fclose(fp);
        }

        ofiles[i].fp = NULL;

        dealloc(&ofiles[i].name);
        dealloc(&ofiles[i].temp);
    }

    ostream = OFILE_PRIMARY;

    dealloc(&last_file);
}


///
///  @brief    Initialize file streams.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void init_files(void)
{
    for (int i = 0; i < IFILE_MAX; ++i)
    {
        ifiles[i].fp  = NULL;
        ifiles[i].eof = false;
        ifiles[i].cr  = false;
    }

    istream = IFILE_PRIMARY;

    for (int i = 0; i < OFILE_MAX; ++i)
    {
        ofiles[i].fp     = NULL;
        ofiles[i].name   = NULL;
        ofiles[i].temp   = NULL;
    }

    ostream = OFILE_PRIMARY;

    last_file = NULL;

    if ((filename_buf = alloc_new(FILENAME_MAX + 1)) == NULL)
    {
        exit(EXIT_FAILURE);
    }

    if (atexit(file_exit) != 0)
    {
        exit(EXIT_FAILURE);
    }
}


///
///  @brief    Open file for input.
///
///  @returns  EXIT_SUCCESS or EXIT_FAILURE.
///
////////////////////////////////////////////////////////////////////////////////

int open_input(const struct tstring *text)
{
    assert(text != NULL);
    
    struct ifile *ifile = &ifiles[istream]; // Current input stream
    FILE *fp = ifile->fp;             

    if (fp != NULL)                     // Stream already open?
    {
        fclose(fp);                     // Yes, so close it
    }

    dealloc(&last_file);

    last_file = alloc_new(text->len + 1);

    sprintf(last_file, "%.*s", (int)text->len, text->buf);

    if ((fp = fopen(last_file, "r")) == NULL)
    {
        return EXIT_FAILURE;
    }

    ifiles[istream].fp  = fp;
    ifiles[istream].eof = false;
    ifiles[istream].cr  = false;

    return EXIT_SUCCESS;
}


///
///  @brief    Open file for output.
///
///  @returns  EXIT_SUCCESS or EXIT_FAILURE.
///
////////////////////////////////////////////////////////////////////////////////

int open_output(const struct tstring *text, int backup)
{
    assert(text != NULL);

    struct ofile *ofile = &ofiles[ostream];
    uint nbytes = text->len;            // Length of text string
    
    dealloc(&ofile->name);
    dealloc(&ofile->temp);
    dealloc(&last_file);

    last_file = alloc_new(nbytes + 1);

    sprintf(last_file, "%.*s", (int)nbytes, text->buf);

    const char *oname = get_oname(ofile, nbytes);

    if ((ofile->fp = fopen(oname, "w+")) == NULL)
    {
        return EXIT_FAILURE;
    }

    ofile->backup = (backup == BACKUP_FILE) ? true : false;

    return EXIT_SUCCESS;
}
