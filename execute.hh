#ifndef EXECUTE_HH
#define EXECUTE_HH

#include <string>
#include <utility>
using std::string;

struct cmd_res_t {
  explicit cmd_res_t()  = default;
  explicit cmd_res_t(int s, string o):
    status(s), output(std::move(o)) {}
  int status{};
  string output;
};

cmd_res_t execute(string cmd);

// compiler output file

struct tmp_file_t {
  explicit tmp_file_t();
  tmp_file_t(const tmp_file_t &) = default;
  ~tmp_file_t();
  char const* path;
  tmp_file_t &operator=(tmp_file_t const& tmp_file);
private:
};

#endif
