#include "execute.hh"
#ifdef DEBUG
#include "print.hh"
#endif

#include <cassert>
#include <cstdlib>  // mkstemp
#include <cstring>  // strdup
#include <unistd.h> // close

cmd_res_t execute(string cmd) {
#ifdef DEBUG
  fprintf(o1, "executing: %s\n", cmd.c_str());
#endif

  cmd += " 2>&1";
  FILE *out = popen(cmd.c_str(), "r");
  if (out == nullptr) {
    perror("popen fails");
    exit(EXIT_FAILURE);
  }

  char out_buf[2048];
  size_t n = fread(out_buf, 1, sizeof(out_buf), out);
  if (ferror(out) != 0) {
    perror("fread error");
    exit(EXIT_FAILURE);
  }
  out_buf[n] = '\0';

  int status = pclose(out);
  if (status == -1) {
    perror("pclose error");
    exit(EXIT_FAILURE);
  }
  status = WEXITSTATUS(status);

  return cmd_res_t(status, out_buf);
}

static char const *get_tmp_file() {
  char *path = strdup("/tmp/osearchXXXXXX");
  int tmp_fd = mkstemp(path);
  close(tmp_fd); // close temp file before executing it to avoid
                 // ETXTBSY
  return path;
}

// compiler output file
tmp_file_t::tmp_file_t() : path(get_tmp_file()) {}
tmp_file_t::~tmp_file_t() {
  if (unlink(path) != 0) {
    perror("unlink failed");
  }
  free(const_cast<char *>(path));
}

tmp_file_t &tmp_file_t::operator=(tmp_file_t const &tmp_file) {
  assert(tmp_file.path == nullptr); // NYI
  return *this;
}
