#include "point.hh"

#include "read_conf.hh" // conf

#include <sstream>
using std::ostringstream;

#include <cstdlib> // rand, strtoul
#include <cassert>
#include <cstddef>

// point_t

point_t::point_t(): val(conf.flags.size(), false) { }

point_t::point_t(string flags_str): val() {
  flags_str.append(" ");
  for (vector<flag_t*>::iterator i = conf.flags.begin();
       i != conf.flags.end(); ++i)
    for (unsigned j = 1; j < (*i)->size(); ++j) {
      string flag = (*i)->get_flag(j);
      flag.append(" ");
      string::size_type loc = flags_str.find(flag);
      if (loc != string::npos) {
	val[unsigned(i-conf.flags.begin())] = j;
	flags_str.erase(flags_str.begin() + ptrdiff_t(loc),
			flags_str.begin() + ptrdiff_t(loc) +
			ptrdiff_t(flag.length()));
      }
    }
  if (flags_str.find_first_not_of(' ') != string::npos) {
    fprintf(stderr, "\nError in configuration file. The following flags have not been found in the flags section: %s\n", flags_str.c_str());
    exit(1);
  }
}

bool point_t::operator==(point_t const& p) const {
  return val == p.val;
}

bool point_t::operator!=(point_t const& p) const {
  return val != p.val;
}

bool point_t::operator<(point_t const& p) const {
  assert(val.size() == p.val.size());

  // if the number of options turned on (size) are unequal, use the
  // size
  unsigned size1 = 0;
  for (val_t::const_iterator i = val.begin();
       i < val.end(); ++i)
    if (*i != 0)
      ++size1;
  unsigned size2 = 0;
  for (val_t::const_iterator i = p.val.begin();
       i < p.val.end(); ++i)
    if (*i != 0)
      ++size2;
  if (size1 != size2)
    return size1 < size2;

  // otherwise use lexicographical order
  for (unsigned i = 0; i < val.size(); ++i) {
    unsigned t1 = val[i];
    unsigned t2 = p.val[i];
    if (t1 != t2)
      return t1 < t2;
  }
  return false;
}

string point_t::str() const {
  string flags_str;
  for (val_t::const_iterator i = val.begin(); i != val.end(); ++i)
    if (*i) {
      flags_str.append(conf.flags[unsigned(i-val.begin())]->get_flag(*i));
      flags_str.append(" ");
    }
  return flags_str;
}

void point_t::set_rand() {
  for(val_t::iterator i = val.begin(); i < val.end(); ++i)
    *i = (rand() % 2 == 0);
}

// obj_t

static int sign(int64_t i) {
  return i == 0 ? 0 : (i < 0 ? -1 : 1);
}

obj_t::obj_t(): inf(), val() {}

obj_t::obj_t(int64_t val): inf(false), val(val) {}

obj_t::obj_t(string str):
  inf(str == "inf"),
  val(inf ? 1 : int64_t(strtoul(str.c_str(), NULL, 0)))
{
  assert(str != "");
}

obj_t obj_t::operator-(obj_t const& obj) const {
  assert((!inf || sign(val) == val) &&
	 (!obj.inf || sign(obj.val) == obj.val));
  return obj_t(inf || obj.inf,
	       (obj.inf ? 0 : val)  - (inf ? 0 : obj.val));
}

bool obj_t::operator==(obj_t const& obj) const {
  return inf == obj.inf && val == obj.val;
}

bool obj_t::operator<(obj_t const& obj) const {
  return 0 < (obj - *this).val;
}

bool obj_t::operator>(obj_t const& obj) const {
  return !(*this < obj || *this == obj);
}

string obj_t::str() const {
  if (inf)
    return string(val < 0 ? "-" : "") + "inf";
  else {
    ostringstream ss;
    ss << val;
    return ss.str();
  }
}

obj_t::obj_t(bool inf, int64_t val): inf(inf), val(inf ? sign(val) : val) {}
