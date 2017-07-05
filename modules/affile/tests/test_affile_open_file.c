/*
 * Copyright (c) 2013 Balabit
 * Copyright (c) 2013 Laszlo Budai
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include "testutils.h"
#include "affile/file-opener.h"
#include "affile/file-specializations.h"
#include "messages.h"

#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define affile_testcase_begin(func, args)	\
  do {						\
    testcase_begin("%s(%s)", func, args);	\
  } while (0)

#define affile_testcase_end() \
  do { 				\
    testcase_end();		\
  } while (0)

#define AFFILE_TESTCASE(testfunc, ...) { affile_testcase_begin(#testfunc, #__VA_ARGS__); testfunc(__VA_ARGS__); affile_testcase_end(); }

#define CREATE_DIRS 0x01
#define PIPE 0x02

#define TEST_DIR "affile_test_dir"

#define PIPE_OPEN_FLAGS (O_RDWR | O_NOCTTY | O_NONBLOCK | O_LARGEFILE)
#define REGULAR_FILE_OPEN_FLAGS (O_CREAT | O_NOCTTY | O_LARGEFILE)

static void
setup(void)
{
  msg_init(FALSE);
  configuration = cfg_new(VERSION_VALUE);
}

static mode_t
get_fd_file_mode(gint fd)
{
  struct stat st;

  fstat(fd, &st);
  return st.st_mode;
}

static gboolean
open_fd(FileOpener *file_opener, gchar *fname, int open_flags, gint extra_flags, gint *fd)
{
  FileOpenerOptions open_opts;

  file_opener_options_defaults(&open_opts);
  file_opener_options_init(&open_opts, configuration);
  open_opts.open_flags = open_flags;
  open_opts.create_dirs = !!(extra_flags & CREATE_DIRS);
  open_opts.needs_privileges = FALSE;

  file_opener_set_options(file_opener, &open_opts);
  return file_opener_open_fd(file_opener, fname, AFFILE_DIR_READ, fd);
}

static gboolean
open_regular_source_file(gchar *fname, int open_flags, gint extra_flags, gint *fd)
{
  FileOpener *file_opener = file_opener_for_regular_source_files_new();
  return open_fd(file_opener, fname, open_flags, extra_flags, fd);
}

static gboolean
open_named_pipe(gchar *fname, int open_flags, gint extra_flags, gint *fd)
{
  FileOpener *file_opener = file_opener_for_named_pipes_new();
  return open_fd(file_opener, fname, open_flags, extra_flags, fd);
}

static void
test_open_regular_file(void)
{
  gint fd;
  gchar fname[] = "test.log";

  assert_true(open_regular_source_file(fname, REGULAR_FILE_OPEN_FLAGS, 0, &fd), "affile_open_regular_source_file failed: %s", fname);
  assert_true(S_ISREG(get_fd_file_mode(fd)) != 0, "%s is not regular file", fname);

  close(fd);
  remove(fname);
}

static void
test_open_named_pipe(void)
{
  gint fd;
  gchar fname[] = "test.pipe";

  assert_true(open_named_pipe(fname, PIPE_OPEN_FLAGS, PIPE, &fd), "failed to open %s", fname);
  assert_true(S_ISFIFO(get_fd_file_mode(fd)) != 0, "%s is not pipe", fname);

  close(fd);
  remove(fname);
}

static void
test_spurious_path(void)
{
  gint fd;
  gchar fname[] = "./../test.fname";

  assert_false(open_regular_source_file(fname, REGULAR_FILE_OPEN_FLAGS, 0, &fd), "affile_open_regular_source_file should not be able to open: %s",
               fname);
}

static void
test_create_file_in_nonexistent_dir(void)
{
  gchar test_dir[] = "nonexistent";
  gchar fname[] = "nonexistent/test.txt";
  gint fd;

  assert_false(open_regular_source_file(fname, REGULAR_FILE_OPEN_FLAGS, 0, &fd), "affile_open_regular_source_file failed: %s", fname);
  assert_true(open_regular_source_file(fname, REGULAR_FILE_OPEN_FLAGS, CREATE_DIRS, &fd), "affile_open_regular_source_file failed: %s", fname);

  close(fd);
  remove(fname);
  rmdir(test_dir);
}

static void
test_file_flags(void)
{
  gint fd;
  gchar fname[] = "test_flags.log";
  gint flags = O_CREAT|O_WRONLY;

  assert_true(open_regular_source_file(fname, flags, 0, &fd), "affile_open_regular_source_file failed: %s", fname);
  assert_true((fcntl(fd, F_GETFL) & O_WRONLY) == O_WRONLY, "invalid open flags");

  close(fd);
  remove(fname);
}

int main(int argc, char **argv)
{
  setup();

  AFFILE_TESTCASE(test_open_regular_file);
  AFFILE_TESTCASE(test_open_named_pipe);
  AFFILE_TESTCASE(test_spurious_path);
  AFFILE_TESTCASE(test_create_file_in_nonexistent_dir);
  AFFILE_TESTCASE(test_file_flags);
}
