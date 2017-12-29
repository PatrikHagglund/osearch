#ifndef READ_CONF_HH
#define READ_CONF_HH

#include "assume.hh"

#include <string>
#include <utility>
#include <vector>

struct flag_t {
  flag_t() = default;
  virtual ~flag_t() = default;
  // FIXME
  flag_t(const flag_t &) = delete;
  flag_t &operator=(flag_t const& tmp_file) = delete;
  flag_t(flag_t&&) = delete;
  flag_t& operator=(flag_t&&) = delete;

  virtual std::string get_flag(unsigned num) const = 0;
  virtual size_t size() const = 0;
  // TODO(uabpath) Implement enough to enable use of range based for loops?
};

struct simple_t: flag_t {
  explicit simple_t(std::string v): value(std::move(v)) {}
  std::string get_flag(unsigned num) const override {
    GNUC_BUILTIN_ASSUME(num == 1);
    return value;
  }
  size_t size() const override {
    return 2;
  }
private:
  std::string value;
};

struct enum_t: flag_t {
  explicit enum_t(const std::string& value) {
    std::string::size_type start = 0;
    for (std::string::size_type loc;
	 (loc = value.find('|', start)) != std::string::npos;
	 start = loc+1) {
      values.push_back(value.substr(start, loc-start));
    }
    values.push_back(value.substr(start));
  }

  std::string get_flag(unsigned num) const override {
    GNUC_BUILTIN_ASSUME(1 <= num && num < size());
    return values[num-1];
  }
  size_t size() const override {
    return values.size()+1;
  }
private:
  std::vector<std::string> values;
};

/* FIXME: Currently unused
struct tuning_t: flag_t {
  unsigned def;
  unsigned min;
  unsigned max;
  unsigned step;
  string separator;
private:
  string value;
};
*/

// conf file data
struct conf_t {
  explicit conf_t() = default;
  std::string prime_command;
  using baselines_t = std::vector<std::string>;
  baselines_t baselines;
  using flags_t = std::vector<flag_t *>;
  flags_t flags;
  std::string get_version;
};

extern conf_t conf;

void read_conf();
void summary_first_read_conf();

#endif // READ_CONF_HH
