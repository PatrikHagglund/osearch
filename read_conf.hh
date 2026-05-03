#ifndef READ_CONF_HH
#define READ_CONF_HH

#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#ifdef DEBUG
#include <sstream>
#include <iomanip>  // std::setw
#endif

/// \file
/// Use read_conf() to read in configuration file into the #conf variable.

/// For flag types simple_t and enum_t.
namespace flag {

/// Representing a compiler option, which may be either present or absent.
struct simple_t {
  explicit simple_t(std::string v): value(std::move(v)) {}
  [[nodiscard]] std::string_view get_flag(unsigned num) const {
    contract_assert(num == 1);
    return value;
  }
  [[nodiscard]] size_t size() const { return 2; }
private:
  std::string value;
};

/// Representing a set of compiler options, for which one is present.
struct enum_t {
  explicit enum_t(const std::string& value) {
    std::string::size_type start = 0;
    for (std::string::size_type loc;
	 (loc = value.find('|', start)) != std::string::npos;
	 start = loc+1) {
      values.push_back(value.substr(start, loc-start));
    }
    values.push_back(value.substr(start));
  }
  [[nodiscard]] std::string_view get_flag(unsigned num) const {
    contract_assert(1 <= num && num < size());
    return values[num-1];
  }
  [[nodiscard]] size_t size() const { return values.size()+1; }
private:
  std::vector<std::string> values;
};

/// A compiler flag: either simple (on/off) or enumerated (one-of-N).
using flag_t = std::variant<simple_t, enum_t>;

/// Get the flag string for value \p num.
[[nodiscard]] inline std::string_view get_flag(const flag_t &f, unsigned num) {
  return std::visit([num](auto &x) { return x.get_flag(num); }, f);
}

/// Number of possible values (including "off").
[[nodiscard]] inline size_t flag_size(const flag_t &f) {
  return std::visit([](auto &x) { return x.size(); }, f);
}
}

/// Type to store all configuration data (in #conf), from the configuration
/// file (by read_conf()). Then used to explore compiler options (by search()).
struct conf_t {
  explicit conf_t() = default;
  /// Compiler command template (used by search()).
  std::string prime_command;
  using baselines_t = std::vector<std::string>;
  /// Compiler baseline commands. Currently unused.
  baselines_t baselines;
  using flags_t = std::vector<flag::flag_t>;
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
       for (size_t j = 1; j < flag::flag_size(i); ++j) {
	 ss << flag::get_flag(i, j) << " ";
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
