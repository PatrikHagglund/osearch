/// \file
/// Use read_conf() to read in configuration file into the #conf variable.

#include "read_conf.hh"
#include "compat.hh"
#include "getopts.hh"
#include "print.hh"

#include <expat.h>

#include <gsl/gsl> // czstring, gsl::owner

#include <iostream> // std::cerr

#include <cstdlib>
#include <cstring>

/// Name of the file containing configuration data.
static gsl::czstring config_file;

/// opt_reg_t function for the config file option.
static void arg_config_file(NONNULL(gsl::czstring) path) {
  std::cout << "arg_config_file:path " << path << '\n';
  config_file = path;
}

/// Option registration static computaion context.
static int dummy_ =
    (opt_reg_t::append(0, arg_config_file, " config_file",
                       "  The 'config_file' describes the search space.\n"),
     1);

void summary_first_read_conf() {
  o3 << "\nConfiguration file: " << config_file;
}

/** Read 'path' to a memory buffer. */
static std::string file_read(gsl::czstring path) {

#ifdef DEBUG
  std::cout << "config file name: " << path << '\n';
#endif

  // open file
  gsl::owner<FILE *> file = fopen(path, "rbe");
  if (file == nullptr) {
    perror("fopen failed");
    _Exit(1);
  }

  // read file size
  if (fseek(file, 0, SEEK_END) != 0) {
    perror("fseek failed");
    _Exit(1);
  }
  long const res = ftell(file);
  if (res == -1) {
    perror("ftell failed");
  }
  if (fseek(file, 0, SEEK_SET) != 0) {
    perror("fseek failed");
    _Exit(1);
  }
  auto size = static_cast<size_t>(res);
#ifdef DEBUG
  std::cout << "config file size: " << size << '\n';
#endif

  // read file
  std::string str(size, '\0');
  size_t const rsize = std::fread(str.data(), sizeof(char), size, file);
  if (rsize != size) {
    perror("fread failed");
    _Exit(1);
  }

  if (fclose(file) != 0) {
    perror("fclose failed");
    _Exit(1);
  }

#ifdef DEBUG
  // std::cout << "file_read:str " << str << '\n';
#endif
  return str;
}

#ifdef DEBUG
static int Depth;
#endif

conf_t conf{}; // start with dummy value

/** Callback function used to parse a single xml element. Used by
    XML_SetElementHandler().

    Store result in the #conf variable. */
static void start(void *data __attribute__((__unused__)), const char *el,
                  const char **attr) {

  // parse get_version
  if (strcmp(el, "get_version") == 0) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (strcmp(attr[0], "value") == 0) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      conf.get_version = attr[1];
    }
  }

  // parse prime command
  if (strcmp(el, "prime") == 0) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (strcmp(attr[0], "command") == 0) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      conf.prime_command = attr[1];
    }
  }

  // parse simple flags
  if (strcmp(el, "flag") == 0) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (strcmp(attr[0], "type") == 0 && strcmp(attr[1], "simple") == 0 &&
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        strcmp(attr[2], "value") == 0) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-owning-memory)
      conf.flags.push_back(new flag::simple_t(attr[3]));
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (strcmp(attr[0], "type") == 0 && strcmp(attr[1], "enum") == 0 &&
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        strcmp(attr[2], "value") == 0) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-owning-memory)
      conf.flags.push_back(new flag::enum_t(attr[3]));
    }
  }

#ifdef DEBUG
  for (int i = 0; i < Depth; i++) {
    std::cout << "  ";
  }

  std::cout << el;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  for (int i = 0; attr[i] != nullptr; i += 2) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::cout << " '" << attr[i] << "'='" << attr[i + 1] << "'";
  }

  std::cout << "\n";
  Depth++;
#endif
}

/** Callback function which is called at the end of a single xml
    element. Used by XML_SetElementHandler(). */
static void end(void *data __attribute__((__unused__)),
                const char *el __attribute__((__unused__))) {
#ifdef DEBUG
  Depth--;
#endif
}

/** Parse the XML configuration file from a memory buffer. */
static void parse(const std::string &conf_file_buf) {

  XML_Parser parser = XML_ParserCreate(nullptr);
  contract_assert(parser != nullptr);

  XML_SetElementHandler(parser, start, end);

  if (XML_Parse(parser, conf_file_buf.c_str(),
                static_cast<int>(conf_file_buf.length()), 1) == 0U) {
    std::cerr << "Parse error at line " << XML_GetCurrentLineNumber(parser)
              << ":\n"
              << XML_ErrorString(XML_GetErrorCode(parser)) << "\n";
    _Exit(EXIT_FAILURE);
  }
}

void read_conf() {
  std::string const conf_file_buf = file_read(config_file);
  parse(conf_file_buf);
#ifdef DEBUG
  std::cout << conf.to_string();
#endif
}
