#ifndef READ_CONF_HH
#define READ_CONF_HH

#include <string>
using std::string;
#include <utility>
#include <vector>
using std::vector;

#include <cassert>

struct flag_t {
  virtual ~flag_t() = default;
  virtual string get_flag(unsigned num) const = 0;
  virtual size_t size() const = 0;
};

struct simple_t: flag_t {
  simple_t(string v): value(std::move(v)) {}
  string value;
  string get_flag(unsigned num) const override {
    assert(num == 1);
    return value;
  }
  size_t size() const override {
    return 2;
  }
};

struct enum_t: flag_t {
  enum_t(const string& value) {
    string::size_type start = 0;
    for (string::size_type loc;
	 (loc = value.find('|', start)) != string::npos;
	 start = loc+1) {
      values.push_back(value.substr(start, loc-start));
}
    values.push_back(value.substr(start));
  }

  vector<string> values;
  string get_flag(unsigned num) const override {
    assert(1 <= num && num < size());
    return values[num-1];
  }
  size_t size() const override {
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
  explicit conf_t() = default;
  string prime_command;
  using baselines_t = vector<string>;
  baselines_t baselines;
  using flags_t = vector<flag_t *>;
  flags_t flags;
  string get_version;
};

extern conf_t conf;

void read_conf();
void summary_first_read_conf();

#endif // READ_CONF_HH
