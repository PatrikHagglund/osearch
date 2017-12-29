#include "compile.hh"

#include "assume.hh"    // for GNUC_BUILTIN_ASSUME
#include "execute.hh"   // for cmd_res_t, tmp_file_t
#include "getopts.hh"   // for opt_reg_t
#include "print.hh"     // for o
#include "read_conf.hh" // for conf

#include <iostream> // for cerr
#include <map>      // for map
#include <string>   // for string
#include <utility>  // for pair

#include <cstdlib> // for EXIT_SUCCESS

std::string input_file;

// BEGIN option handling for 'input_add'

static std::string input_add;

static void opt_i() { input_add = std::string(optarg) + " "; }

static int _dummy =
    (opt_reg_t::append(
         'i', &opt_i, "i:", " [-i in_opts]",
         "  -i in_opts \textra options for processing 'code_file'\n"),
     1);

// END

void summary_first_compile() {
  o3 << "\nInput file: " << input_file;
  o3 << "\nBase command: " << conf.prime_command;
  if (!input_add.empty()) {
    o3 << "\nInput options: " << input_add;
  }
}

pset_t error_pset() { return 0; }

// Construct command string
static std::string point_to_cmd(const point_t &p,
                                const std::string_view tmp_path) {

  std::string cmd = conf.prime_command;

  std::string prefix = "OSEARCH_";

  // Replace OSEARCH_IN
  std::string inp_str = prefix + "IN";
  std::string::size_type loc = cmd.find(inp_str);
  cmd.replace(loc, inp_str.size(), input_add + input_file);

  // Replace OSEARCH_OUT
  std::string outp_str = prefix + "OUT";
  loc = cmd.find(outp_str);
  cmd.replace(loc, outp_str.size(), tmp_path);

  // Replace OSEARCH_OPT
  std::string opt_str = prefix + "OPTS";
  loc = cmd.find(opt_str);
  cmd.replace(loc, opt_str.size(), p.str());

  return cmd;
}

static std::string bugpoint(point_t p, cmd_res_t const &cmd_res,
                            const std::string_view tmp_path) {
  for (auto const &i : p.val) {
    if (i != 0U) {
      point_t new_p = p;
      new_p.val[&i - &*p.val.cbegin()] = static_cast<unsigned char>(i == 0U);
      cmd_res_t new_cmd_res = execute(point_to_cmd(new_p, tmp_path));
      if (new_cmd_res.status != EXIT_SUCCESS &&
          new_cmd_res.output == cmd_res.output) {
        p = new_p;
      }
    }
  }
  std::string cmd = point_to_cmd(p, tmp_path);
  o1 << "\nMinimal option string that generates the same error:\n" << cmd;
  o1 << "\nCommand output:\n" << cmd_res.output;
  return cmd;
}

// current "minimal" option string for producing this file
using pset_to_point_t = std::map<pset_t, point_t>;
static pset_to_point_t pset_to_point;

// map points to sets of equivalent points
using point_to_pset_t = std::map<point_t, pset_t>;
static point_to_pset_t point_to_pset;

// handels to all executable files
struct exe_files_t {
private:
  using m_t = std::map<pset_t, tmp_file_t>;
  // storage
  // a map from the set of points to tmp_files (executable files)
  m_t m_;

public:
  explicit exe_files_t() = default;
  void reset() {
    for (auto &i : m_) {
      i.second.reset_path();
    }
  }
  [[nodiscard]] const m_t &m() const { return m_; } // getter
  m_t &m() { return m_; }                           // getter/setter
};
static exe_files_t exe_files;

static bool file_equal(const std::string_view path1,
                       const std::string_view path2) {
  std::string cmd =
      std::string("cmp ") + std::string(path1) + " " + std::string(path2);
  cmd_res_t cmd_res = execute(cmd);
  return cmd_res.status == 0;
}

static unsigned last_pset = 0;

static pset_t get_pset(const std::string_view tmp_path) {
  for (auto &i : exe_files.m()) {
    if (file_equal(tmp_path, i.second.get_path())) {
      return i.first;
    }
  }
  return ++last_pset;
}

static void hard_compile(const point_t &p) {
  tmp_file_t tmp_file;
  std::string cmd = point_to_cmd(p, tmp_file.get_path());

#if 0
#ifdef DEBUG
  progress.tick('o', p);
#else
  progress.tick('o');
#endif
#endif

  cmd_res_t cmd_res = execute(cmd);

  if (cmd_res.status != EXIT_SUCCESS) {
    std::cerr << "\nExectuion of '" << cmd
              << "' failed with status code: " << cmd_res.status << "\n";
    bugpoint(p, cmd_res, tmp_file.get_path());
    point_to_pset[p] = error_pset();
    return;
  }

  pset_t pset = get_pset(tmp_file.get_path());

  GNUC_BUILTIN_ASSUME(point_to_pset.find(p) == point_to_pset.end());
  point_to_pset[p] = pset;

  if (exe_files.m().find(pset) == exe_files.m().end()) {
    exe_files.m().insert(std::pair<pset_t, tmp_file_t const &>(pset, tmp_file));
    tmp_file.reset_path();
  }
}

// compile on demand
pset_t compile(const point_t &p) {
  if (point_to_pset.find(p) == point_to_pset.end()) {
    hard_compile(p);
  }
  pset_t pset = point_to_pset[p];
  GNUC_BUILTIN_ASSUME(exe_files.m().find(pset) != exe_files.m().end());

  // update pset-to-point mapping as side-effect
  if (pset_to_point.find(pset) == pset_to_point.end() ||
      p < pset_to_point[pset]) {
    pset_to_point[pset] = p;
  }

  return pset;
}

tmp_file_t const &get_tmp_file(pset_t p) {
  GNUC_BUILTIN_ASSUME(exe_files.m().find(p) != exe_files.m().end());
  return exe_files.m().find(p)->second;
}

point_t get_point(pset_t pset) {
  GNUC_BUILTIN_ASSUME(pset_to_point.find(pset) != pset_to_point.end());
  return pset_to_point[pset];
}

bool equivalent_p(const point_t &p1, const point_t &p2) {
  GNUC_BUILTIN_ASSUME(point_to_pset.find(p1) != point_to_pset.end() &&
                      point_to_pset.find(p2) != point_to_pset.end());
  return point_to_pset[p1] == point_to_pset[p2];
}

void reset_compilations() {
  last_pset = 0;
  point_to_pset = point_to_pset_t();
  exe_files.reset();
  exe_files = exe_files_t();
}

std::string get_compiler_version() {
  cmd_res_t cmd_res = execute(conf.get_version);
  std::string str = cmd_res.output;
  str.erase(str.end() - 1); // delete '\n'
  return str;
}
