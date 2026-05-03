/// \file
/// Runtime tests for read_conf (flag types and XML parsing into conf_t).
///
/// Links read_conf.cc and getopts.cc. Writes a temporary XML config file,
/// invokes get_opts() to set the config_file path, then calls read_conf()
/// and verifies the resulting conf global.

#include "../read_conf.hh"
#include "../getopts.hh"
#include "../print.hh"

#include "check.hh"

#include <cstdio>
#include <cstring>
#include <string>

// --- Stubs ------------------------------------------------------------------

progress_t progress{};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-declarations"
void progress_t::tick(char) {}
void progress_t::newl() const {}
void progress_t::print_symbols() const {}
#pragma GCC diagnostic pop

// --- Flag type tests (header-only, no XML needed) ---------------------------

static void test_simple_flag() {
  flag::simple_t f{"-O2"};
  CHECK_EQ(f.size(), size_t{2}); // 0=off, 1=on
  CHECK(f.get_flag(1) == std::string{"-O2"});
}

static void test_enum_flag() {
  flag::enum_t f{"-O0|-O1|-O2|-O3"};
  // size = number of values + 1 (for "off")
  CHECK_EQ(f.size(), size_t{5});
  CHECK(f.get_flag(1) == std::string{"-O0"});
  CHECK(f.get_flag(2) == std::string{"-O1"});
  CHECK(f.get_flag(3) == std::string{"-O2"});
  CHECK(f.get_flag(4) == std::string{"-O3"});
}

static void test_enum_flag_two_values() {
  flag::enum_t f{"-march=native|-march=generic"};
  CHECK_EQ(f.size(), size_t{3});
  CHECK(f.get_flag(1) == std::string{"-march=native"});
  CHECK(f.get_flag(2) == std::string{"-march=generic"});
}

// --- XML parsing via read_conf() --------------------------------------------

static void test_read_conf_parses_xml() {
  // Write a minimal config file.
  char path[] = "/tmp/osearch_test_conf_XXXXXX";
  int fd = mkstemp(path);
  CHECK(fd >= 0);

  static constexpr char xml[] =
      "<?xml version=\"1.0\"?>\n"
      "<osearch>\n"
      "  <get_version value=\"gcc --version\"/>\n"
      "  <prime command=\"gcc -O2\"/>\n"
      "  <flag type=\"simple\" value=\"-funroll-loops\"/>\n"
      "  <flag type=\"simple\" value=\"-finline-functions\"/>\n"
      "  <flag type=\"enum\" value=\"-O0|-O1|-O2|-O3\"/>\n"
      "</osearch>\n";

  auto written = write(fd, xml, std::strlen(xml));
  CHECK(written == static_cast<ssize_t>(std::strlen(xml)));
  close(fd);

  // Reset conf before parsing.
  conf = conf_t{};

  // Use get_opts to set config_file (positional arg 0).
  gsl::not_null<gsl::czstring> argv[] = {"test_read_conf", path};
  // Reset getopt state.
  optind = 1;
  get_opts(argv);

  read_conf();

  CHECK(conf.get_version == std::string{"gcc --version"});
  CHECK(conf.prime_command == std::string{"gcc -O2"});
  CHECK_EQ(conf.flags.size(), size_t{3});

  // First two are simple flags.
  CHECK_EQ(flag::flag_size(conf.flags[0]), size_t{2});
  CHECK(flag::get_flag(conf.flags[0], 1) == std::string{"-funroll-loops"});
  CHECK_EQ(flag::flag_size(conf.flags[1]), size_t{2});
  CHECK(flag::get_flag(conf.flags[1], 1) == std::string{"-finline-functions"});

  // Third is an enum flag with 4 values.
  CHECK_EQ(flag::flag_size(conf.flags[2]), size_t{5});
  CHECK(flag::get_flag(conf.flags[2], 1) == std::string{"-O0"});
  CHECK(flag::get_flag(conf.flags[2], 4) == std::string{"-O3"});

  unlink(path);
}

// --- Driver -----------------------------------------------------------------

int main() {
  test_simple_flag();
  test_enum_flag();
  test_enum_flag_two_values();
  test_read_conf_parses_xml();
  return TEST_REPORT();
}
