#include "execute.hh"
#ifdef DEBUG
#include "print.hh"
#endif

#include <unistd.h> // close

#include <cstdlib> // mkstemp
#include <cstring> // srncpy

#include <array>    // std::array
#include <iostream> // std::cout

cmd_res_t execute(std::string cmd) {
#ifdef DEBUG
  o1 << "executing: " << cmd << "\n";
#endif

  cmd += " 2>&1";
  FILE *out = popen(cmd.c_str(), "r"); // NOLINT(cert-env33-c)
  if (out == nullptr) {
    perror("popen fails");
    exit(EXIT_FAILURE);
  }

  constexpr unsigned out_buf_maxlen = 2048;
  std::array<char, out_buf_maxlen> out_buf{};
  size_t n = fread(&out_buf[0], 1, out_buf.size(), out);
  if (ferror(out) != 0) {
    perror("fread error");
    exit(EXIT_FAILURE);
  }
  out_buf[n++] = '\0';
  std::string_view out_str(&out_buf[0], n);

  int status = pclose(out);
  if (status == -1) {
    perror("pclose error");
    exit(EXIT_FAILURE);
  }
  status = WEXITSTATUS(status); // NOLINT(hicpp-signed-bitwise)

  return cmd_res_t(status, out_str);
}

// compiler output file
tmp_file_t::tmp_file_t() {
  std::strncpy(&path[0], &templ[0], sizeof(path));
  int tmp_fd = mkstemp(&path[0]);
  close(tmp_fd); // close temp file before executing it to avoid
                 // ETXTBSY
}
