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
  // Move-only: this owns a temp file that the destructor unlink()s, so
  // copying would double-unlink (and leave a dangling path). Transfer
  // ownership by move; the moved-from object is disarmed (path[0]='\0').
  tmp_file_t(const tmp_file_t &) = delete;
  tmp_file_t &operator=(tmp_file_t const&) = delete;
  tmp_file_t(tmp_file_t&& o) noexcept {
    std::memcpy(path, o.path, sizeof(path));
    o.path[0] = '\0';
  }
  tmp_file_t& operator=(tmp_file_t&& o) noexcept {
    if (this != &o) {
      if (path[0] != '\0') unlink(&path[0]);
      std::memcpy(path, o.path, sizeof(path));
      o.path[0] = '\0';
    }
    return *this;
  }
private:
  static constexpr char templ[] = "/tmp/osearchXXXXXX";
  char path[sizeof(templ)] = "";
public:
  /// Getter for tmp_file_t#path.
  [[nodiscard]] std::string_view get_path() const {
    return std::string_view(&path[0], std::strlen(&path[0]));
  }
  /// Disarm: forget the path without unlinking (used when ownership of the
  /// file has been transferred elsewhere, or the file is intentionally kept).
  constexpr void reset_path() {
    path[0] = '\0';
  }
};

#endif
