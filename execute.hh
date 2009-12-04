#ifndef EXECUTE_HH
#define EXECUTE_HH

#include <string>
using std::string;

struct cmd_res_t {
  explicit cmd_res_t(): status(), output() {};
  explicit cmd_res_t(int status, string output):
    status(status), output(output) {};
  int status;
  string output;
};

cmd_res_t execute(string cmd);

// compiler output file

struct tmp_file_t {
  explicit tmp_file_t();
  ~tmp_file_t();
  char const* path;
private:
  // unused
  tmp_file_t(tmp_file_t const& tmp_file);
  tmp_file_t const& operator=(tmp_file_t const& tmp_file);
};

#endif
