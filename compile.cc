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

/// \file
/// Compile given a point_t, using compile(). Store results in
/// exe_files, point_to_pset, and pset_to point mappings.

std::string input_file;

// BEGIN option handling for 'input_add'

/// Extra input options, passed by command line option (-i in_opts).
static std::string input_add;

/// Option helper function.
/// \todo Possible to use lambda function?
static void opt_i() { input_add = std::string(optarg) + " "; }

/// Option helper varaible.
static int dummy_ =
    (opt_reg_t::append(
         'i', &opt_i, "i:", " [-i in_opts]",
         "  -i in_opts \textra options for compiling 'code_file'\n"),
     1);

// END

void summary_first_compile() {
  o3 << "\nInput file: " << input_file;
  o3 << "\nBase command: " << conf.prime_command;
  if (!input_add.empty()) {
    o3 << "\nInput options: " << input_add;
  }
}

/// Construct command string, given a point_t and a output file name.
/// \param p a point_t option list
/// \param tmp_path output file
/// \return command string
static std::string point_to_cmd(const point_t &p,
                                const std::string_view tmp_path) {

  std::string cmd = conf.prime_command;

  std::string const prefix = "OSEARCH_";

  // Replace OSEARCH_IN
  std::string const inp_str = prefix + "IN";
  std::string::size_type loc = cmd.find(inp_str);
  cmd.replace(loc, inp_str.size(), input_add + input_file + " -lm -lrt");

  // Replace OSEARCH_OUT
  std::string const outp_str = prefix + "OUT";
  loc = cmd.find(outp_str);
  cmd.replace(loc, outp_str.size(), tmp_path);

  // Replace OSEARCH_OPT
  std::string const opt_str = prefix + "OPTS";
  loc = cmd.find(opt_str);
  cmd.replace(loc, opt_str.size(), p.to_string());

  return cmd;
}

/// Try to minimize the command string that gives the same cmd_res_t.
/// \param p point_t
/// \param cmd_res cmd_res_t
/// \param tmp_path output file name
/// \return command string
static std::string bugpoint(point_t p, cmd_res_t const &cmd_res,
                            const std::string_view tmp_path) {
  for (auto const &i : p.val) {
    if (i != 0U) {
      point_t new_p = p;
      new_p.val[&i - &*p.val.cbegin()] = static_cast<unsigned char>(i == 0U);
      cmd_res_t const new_cmd_res = execute(point_to_cmd(new_p, tmp_path));
      /// \todo equal operator?
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

/// Current "minimal" option string for producing this file.
using pset_to_point_t = std::map<pset_t, point_t>;
/// Current "minimal" option string for producing this file.
static pset_to_point_t pset_to_point;

point_t get_point(pset_t pset) {
  GNUC_BUILTIN_ASSUME(pset_to_point.contains(pset));
  return pset_to_point[pset];
}

/// Map points to sets of equivalent points.
using point_to_pset_t = std::map<point_t, pset_t>;
/// Map points to sets of equivalent points.
static point_to_pset_t point_to_pset;

bool equivalent_p(const point_t &p1, const point_t &p2) {
  GNUC_BUILTIN_ASSUME(point_to_pset.contains(p1) &&
                      point_to_pset.contains(p2));
  return point_to_pset[p1] == point_to_pset[p2];
}

/// Compare the content of two output files.
/// \param path1 Output file name
/// \param path2 Output file name
/// \return true if the content is equal, false otherwise
static bool file_equal(const std::string_view path1,
                       const std::string_view path2) {
  std::string const cmd =
      std::string("cmp ") + std::string(path1) + " " + std::string(path2);
  cmd_res_t const cmd_res = execute(cmd);
  return cmd_res.status == 0;
}

/// Store handles to all executable files.
struct exe_files_t {
private:
  using m_t = std::map<pset_t, tmp_file_t>;
  /// Storage consisting of a map from the set of points to tmp_files
  /// (generated programs).
  m_t m_;

public:
  explicit exe_files_t() = default;
  void reset() {
    for (auto &i : m_) {
      i.second.reset_path();
    }
  }
  /// Getter for exe_files_t#m_.
  [[nodiscard]] const m_t &m() const { return m_; }
  /// Getter/setter for exe_files_t#m_.
  m_t &m() { return m_; }
};
/// Store handles to all executable files.
static exe_files_t exe_files;

/// Last unique pset number/id.
static unsigned last_pset = pset_invalid;

/// Check if an equal file exist, and return the corresponding pset_t
/// number/id. Otherwise return an new unique pset_t number/id.
static pset_t get_pset(const std::string_view tmp_path) {
  for (auto &i : exe_files.m()) {
    if (file_equal(tmp_path, i.second.get_path())) {
      return i.first;
    }
  }
  return ++last_pset;
}

tmp_file_t const &get_tmp_file(pset_t p) {
  GNUC_BUILTIN_ASSUME(exe_files.m().contains(p));
  return exe_files.m().find(p)->second;
}

/// Given a point_t, compile (generate code) and insert the result in
/// the set of exe_files_t.
/// \param p point_t
static void hard_compile(const point_t &p) {
  tmp_file_t tmp_file;
  std::string const cmd = point_to_cmd(p, tmp_file.get_path());

  cmd_res_t const cmd_res = execute(cmd);

  if (cmd_res.status != EXIT_SUCCESS) {
    std::cerr << "\nExectuion of '" << cmd
              << "' failed with status code: " << cmd_res.status << "\n";
    bugpoint(p, cmd_res, tmp_file.get_path());
    point_to_pset[p] = pset_invalid;
    return;
  }

  pset_t const pset = get_pset(tmp_file.get_path());

  GNUC_BUILTIN_ASSUME(!point_to_pset.contains(p));
  point_to_pset[p] = pset;

  if (!exe_files.m().contains(pset)) {
    exe_files.m().insert(std::pair<pset_t, tmp_file_t const &>(pset, tmp_file));
    tmp_file.reset_path();
  }
}

pset_t compile(const point_t &p) {
  if (!point_to_pset.contains(p)) {
    hard_compile(p);
  }
  pset_t const pset = point_to_pset[p];
  GNUC_BUILTIN_ASSUME(exe_files.m().contains(pset));

  /// Update pset-to-point mapping as a side-effect.
  if (!pset_to_point.contains(pset) ||
      p < pset_to_point[pset]) {
    pset_to_point[pset] = p;
  }

  return pset;
}

/// Reset the set of compiled files (exe_files).
/// \todo Do we need to reset pset_to_point?
/// \todo Make a member function.
void reset_compilations() {
  last_pset = 0;
  point_to_pset = point_to_pset_t();
  exe_files.reset();
  exe_files = exe_files_t();
}

std::string get_compiler_version() {
  cmd_res_t const cmd_res = execute(conf.get_version);
  std::string str = cmd_res.output;
  str.erase(str.end() - 1); // delete '\n'
  return str;
}
