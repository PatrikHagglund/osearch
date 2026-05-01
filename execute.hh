#ifndef EXECUTE_HH
#define EXECUTE_HH

#include <unistd.h> // unlink

#include <string>
#include <utility>
#include <cstring>

/// \file
/// Execute a system command using execute().

/// Exit status and output string from a system command.
struct cmd_res_t {
  explicit cmd_res_t()  = default;
  explicit cmd_res_t(int s, std::string_view o):
    status(s), output(o) {}
  int status{};
  std::string output;
};

/// Execute a system command.
/// \param cmd command
/// \return exit status and output string in cmd_res_t
cmd_res_t execute(std::string cmd);


/// Temporary file (compiler output file).
struct tmp_file_t {
  explicit tmp_file_t();
  ~tmp_file_t() {
    if (path[0] != '\0') {
      if (unlink(&path[0]) != 0) {
	perror("unlink failed");
      }
    }
  }
  // FIXME
  tmp_file_t(const tmp_file_t &) = default;
  tmp_file_t &operator=(tmp_file_t const& tmp_file) = default;
  tmp_file_t(tmp_file_t&&) = default;
  tmp_file_t& operator=(tmp_file_t&&) = default;
private:
  static constexpr char templ[] = "/tmp/osearchXXXXXX";
  char path[sizeof(templ)] = "";
public:
  /// Getter for tmp_file_t#path.
  [[nodiscard]] std::string_view get_path() const {
    return std::string_view(&path[0], std::strlen(&path[0]));
  }
  /// Reset the path name, to the empty string.
  constexpr void reset_path() {
    path[0] = '\0'; // hack, until we implement move constructor
  }
};

#endif
