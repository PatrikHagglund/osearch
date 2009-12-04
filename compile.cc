#include "compile.hh"

#include "print.hh" // o
#include "read_conf.hh" // conf
#include "getopts.hh" // opt_init_t

#include <map>
using std::map;
#include <utility>
using std::pair;
using std::make_pair;

#include <cstdlib> // EXIT_SUCCESS

string input_file;

// BEGIN option handling for 'input_add'

static string input_add;

static void opt_i() {
  input_add = string(optarg) + " ";
}

static opt_reg_t
opt_reg('i', &opt_i, "i:", " [-i in_opts]",
	"  -i in_opts \textra options for processing 'code_file'\n");

// END

void summary_first_compile() {
  fprintf(o3, "\nInput file: %s", input_file.c_str());
  fprintf(o3, "\nBase command: %s", conf.prime_command.c_str());
  if (input_add.size() > 0)
    fprintf(o3, "\nInput options: %s", input_add.c_str());
}

pset_t error_pset() {
  return 0;
}

// Construct command string
static string point_to_cmd(point_t p, string tmp_path) {

  string cmd = conf.prime_command;

  string prefix = "OSEARCH_";

  // Replace OSEARCH_IN
  string inp_str = prefix + "IN";
  string::size_type loc = cmd.find(inp_str);
  cmd.replace(loc, inp_str.size(), input_add + input_file);

  // Replace OSEARCH_OUT
  string outp_str = prefix + "OUT";
  loc = cmd.find(outp_str);
  cmd.replace(loc, outp_str.size(), tmp_path);

  // Replace OSEARCH_OPT
  string opt_str = prefix + "OPTS";
  loc = cmd.find(opt_str);
  cmd.replace(loc, opt_str.size(), p.str());

  return cmd;
}

static string bugpoint(point_t p, cmd_res_t cmd_res, string tmp_path) {
  for(point_t::val_t::iterator i = p.val.begin(); i < p.val.end(); ++i) {
    if (*i) {
      point_t new_p = p;
      new_p.val[unsigned(i-p.val.begin())] = !*i;
      cmd_res_t new_cmd_res = execute(point_to_cmd(new_p, tmp_path));
      if (new_cmd_res.status != EXIT_SUCCESS &&
	  new_cmd_res.output.compare(cmd_res.output) == 0) {
	p = new_p;
      }
    }
  }
  string cmd = point_to_cmd(p, tmp_path);
  fprintf(o1, "\nMinimal option string that generates the same error:\n%s",
	 cmd.c_str());
  fprintf(o1, "\nCommand output:\n%s", cmd_res.output.c_str());
  return cmd;
}

// current "minimal" option string for producing this file
typedef map<pset_t, point_t> pset_to_point_t;
pset_to_point_t pset_to_point;

// map points to sets of equivalent points
typedef map<point_t, pset_t> point_to_pset_t;
point_to_pset_t point_to_pset;

// handels to all executable files
struct exe_files_t {
  typedef map<pset_t, tmp_file_t const&> m_t;
  m_t m;
  explicit exe_files_t(): m() {}
  void clean() {
    for (m_t::const_iterator i = m.begin(); i != m.end(); ++i)
      delete &(i->second);
  };
  ~exe_files_t() {
    clean();
  }
};
exe_files_t exe_files;

static bool file_equal(string path1, string path2) {
  string cmd = string("cmp ") + path1 + " " + path2;
  cmd_res_t cmd_res = execute(cmd);
  return cmd_res.status == 0;
}

static unsigned last_pset = 0;

static pset_t get_pset(string tmp_path) {
  for (exe_files_t::m_t::const_iterator i = exe_files.m.begin();
       i != exe_files.m.end(); ++i) {
    if (file_equal(tmp_path, i->second.path)) {
      return i->first;
    }
  }
  return ++last_pset;
}

static void hard_compile(point_t p) {
  tmp_file_t const& tmp_file = *(new tmp_file_t);
  string cmd = point_to_cmd(p, tmp_file.path);

#if 0
#ifdef DEBUG
  progress.tick('o', p);
#else
  progress.tick('o');
#endif
#endif

  cmd_res_t cmd_res = execute(cmd);

  if (cmd_res.status != EXIT_SUCCESS) {
    fprintf(stderr, "\nExectuion of '%s' failed with status code: %d\n",
	    cmd.c_str(), cmd_res.status);
    bugpoint(p, cmd_res, tmp_file.path);
    point_to_pset[p] = error_pset();
    return;
  }

  pset_t pset = get_pset(tmp_file.path);

  assert(point_to_pset.find(p) == point_to_pset.end());
  point_to_pset[p] = pset;

  if (exe_files.m.find(pset) == exe_files.m.end())
    exe_files.m.insert(pair<pset_t, tmp_file_t const&>(pset, tmp_file));
  else
    delete &tmp_file;
}

// compile on demand
pset_t compile(point_t p) {
  if (point_to_pset.find(p) == point_to_pset.end())
    hard_compile(p);
  pset_t pset = point_to_pset[p];
  assert(exe_files.m.find(pset) != exe_files.m.end());

  // update pset-to-point mapping as side-effect
  if (pset_to_point.find(pset) == pset_to_point.end()
      || p < pset_to_point[pset])
    pset_to_point[pset] = p;

  return pset;
}

tmp_file_t const& get_tmp_file(pset_t p) {
  assert(exe_files.m.find(p) != exe_files.m.end());
  return exe_files.m.find(p)->second;
}

point_t get_point(pset_t pset) {
  assert(pset_to_point.find(pset) != pset_to_point.end());
  return pset_to_point[pset];
}

bool equivalent_p(point_t p1, point_t p2) {
  assert(point_to_pset.find(p1) != point_to_pset.end() &&
	 point_to_pset.find(p2) != point_to_pset.end());
  return point_to_pset[p1] == point_to_pset[p2];
}

void reset_compilations() {
  last_pset = 0;
  point_to_pset = point_to_pset_t();
  exe_files.clean();
  exe_files = exe_files_t();
}

string get_compiler_version() {
  cmd_res_t cmd_res = execute(conf.get_version);
  string str = cmd_res.output;
  str.erase(str.end()-1); // delete '\n'
  return str;
}
