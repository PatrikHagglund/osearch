#ifndef READ_CONF_HH
#define READ_CONF_HH

#include "assume.hh"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef DEBUG
#include <sstream>
#include <iomanip>  // std::setw
#endif

/// \file
/// Use read_conf() to read in configuration file into the #conf variable.

/// For flag_t and its derived classes simple_t and enum_t.
namespace flag {
/// Abstract base class for the compiler option types (currently simple_t and enum_t).
struct flag_t {
  flag_t() = default;
  virtual ~flag_t() = default;
  // FIXME
  flag_t(const flag_t &) = delete;
  flag_t &operator=(flag_t const& tmp_file) = delete;
  flag_t(flag_t&&) = delete;
  flag_t& operator=(flag_t&&) = delete;

  [[nodiscard]] virtual std::string_view get_flag(unsigned num) const = 0;
  [[nodiscard]] virtual size_t size() const = 0;
  // TODO(uabpath) Implement enough to enable use of range based for loops?
};

/// Representing a compiler option, which may be either present or absent.
struct simple_t: flag_t {
  explicit simple_t(std::string v): value(std::move(v)) {}
  [[nodiscard]] std::string_view get_flag(unsigned num) const override {
    contract_assert(num == 1);
    return value;
  }
  [[nodiscard]] size_t size() const override {
    return 2;
  }
private:
  std::string value;
};

/// Representing a set of compiler options, for which one is present.
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

  [[nodiscard]] std::string_view get_flag(unsigned num) const override {
    contract_assert(1 <= num && num < size());
    return values[num-1];
  }
  [[nodiscard]] size_t size() const override {
    return values.size()+1;
  }
private:
  std::vector<std::string> values;
};
}

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

/// Type to store all configuration data (in #conf), from the configuration
/// file (by read_conf()). Then used to explore compiler options (by search()).
struct conf_t {
  explicit conf_t() = default;
  /// Compiler command template (used by search()).
  std::string prime_command;
  using baselines_t = std::vector<std::string>;
  /// Compiler baseline commands. Currently unused.
  baselines_t baselines;
  using flags_t = std::vector<NONNULL(flag::flag_t *)>;
  /// List of compiler options (used by search()).
  flags_t flags;
  /// Compiler command to get its version (used by get_compiler_version()).
  std::string get_version;
#ifdef DEBUG
  std::string to_string()
    {
     std::ostringstream ss;
     ss << "prime command: " << prime_command << "\n"
     << "baselines:\n";
     for (auto const &i : baselines) {
       ss << i << "\n";
     }
     ss << "flags:\n";
     for (auto const &i : flags) {
       ss << std::setw(3) << &i - &*flags.cbegin() << ": ";
       for (size_t j = 1; j < i->size(); ++j) {
	 ss << i->get_flag(j) << " ";
       }
       ss << "\n";
     }
     return ss.str();
  }
#endif
};

/// Global holder of the configuration values (after read_conf()). See description at conf_t.
extern conf_t conf;

/// Read the configuration file #config_file, and parse it into #conf.
void read_conf();

/// Print out the configuration file name.
void summary_first_read_conf();

#endif // READ_CONF_HH
