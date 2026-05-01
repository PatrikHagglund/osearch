#include "execute.hh"
#ifdef DEBUG
#include "print.hh"
#endif

#include <unistd.h> // close

#include <cstdlib> // mkstemp
#include <cstring> // srncpy

#include <array>    // std::array
#include <iostream> // std::cout

/// \file
/// Execute a system command using execute().

cmd_res_t execute(std::string cmd) {
#ifdef DEBUG
  o1 << "executing: " << cmd << "\n";
#endif

  cmd += " 2>&1";
  FILE *out = popen(cmd.c_str(), "r"); // NOLINT(cert-env33-c)
  if (out == nullptr) {
    perror("popen fails");
    _Exit(EXIT_FAILURE);
  }

  constexpr unsigned out_buf_maxlen = 2048;
  std::array<char, out_buf_maxlen> out_buf{};
  size_t n = fread(out_buf.data(), 1, out_buf.size(), out);
  if (ferror(out) != 0) {
    perror("fread error");
    _Exit(EXIT_FAILURE);
  }
  out_buf[n++] = '\0';
  std::string_view const out_str(out_buf.data(), n);

  int status = pclose(out);
  if (status == -1) {
    perror("pclose error");
    _Exit(EXIT_FAILURE);
  }
  status = WEXITSTATUS(status); // NOLINT(hicpp-signed-bitwise)

  return cmd_res_t(status, out_str);
}

/// Set tmp_file_t#path to a filer name of a temporary file (compiler output
/// file).
tmp_file_t::tmp_file_t() {
  std::strncpy(&path[0], &templ[0], sizeof(path));
  int const tmp_fd = mkstemp(&path[0]);
  close(tmp_fd); // close temp file before executing it to avoid
                 // ETXTBSY
}
