#ifndef _READ_CONF_HH
#define _READ_CONF_HH

#include <string>
using std::string;
#include <vector>
using std::vector;

#include <cassert>

struct flag_t {
  virtual ~flag_t() {}
  virtual string get_flag(unsigned num) const = 0;
  virtual unsigned size() const = 0;
};

struct simple_t: flag_t {
  simple_t(string value): value(value) {}
  string value;
  string get_flag(unsigned num) const {
    assert(num == 1);
    return value;
  }
  unsigned size() const {
    return 2;
  }
};

struct enum_t: flag_t {
  enum_t(string value): values() {
    string::size_type start = 0;
    for (string::size_type loc;
	 (loc = value.find('|', start)) != string::npos;
	 start = loc+1)
      values.push_back(value.substr(start, loc-start));
    values.push_back(value.substr(start));
  }

  vector<string> values;
  string get_flag(unsigned num) const {
    assert(1 <= num && num < size());
    return values[num-1];
  }
  unsigned size() const {
    return values.size()+1;
  }
};

struct tuning_t: flag_t {
  string value;
  unsigned def;
  unsigned min;
  unsigned max;
  unsigned step;
  string separator;
};

// conf file data
struct conf_t {
  explicit conf_t(): prime_command(), baselines(), flags(), get_version() {}
  string prime_command;
  typedef vector<string> baselines_t;
  baselines_t baselines;
  typedef vector<flag_t*> flags_t;
  flags_t flags;
  string get_version;
};

extern conf_t conf;

void read_conf();
void summary_first_read_conf();

#endif // _READ_CONF_HH
