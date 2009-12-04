/** \file read_conf.cc Use read_conf() to read in configuration file
    into the #conf variable. */

#include "read_conf.hh"
#include "getopts.hh"
#include "print.hh"

#include <expat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

static char const *config_file;

static void arg_config_file(char const *path) {
  config_file = path;
}

opt_reg_t opt_reg(0, arg_config_file, " config_file",
		  "  The 'config_file' describes the search space.\n");

void summary_first_read_conf() {
  fprintf(o3, "\nConfiguration file: %s", config_file);
}

struct str_array {
  char const * str;
  size_t len;
};

/** Read 'path' to a memory buffer. */
static str_array file_read(char const * path) {

  // open file
  FILE * file = fopen(path, "rb");
  if (file == NULL) {
    perror("fopen failed");
    exit(1);
  }

  // read file size
  if (fseek(file, 0, SEEK_END) != 0) {
    perror("fseek failed");
    exit(1);
  }
  long res = ftell(file);
  if (res == -1)
    perror("ftell failed");
  size_t size = size_t(res);
  rewind(file);

  // read file
  char * file_buf = (char *)malloc(size + 1);
  size_t rsize = fread(file_buf, sizeof(char), size, file);
  if (rsize != size) {
    perror("fread failed");
    exit(1);
  }

  if (fclose(file)) {
    perror("fclose failed");
    exit(1);
  }

  file_buf[size] = '\0'; // write terminating null byte after the end

  str_array ret = { file_buf, size };
  return ret;
}

#if DEBUG
int Depth;
#endif

/** Holds the configuration values (after the parse) */
conf_t conf = conf_t(); // start with dummy value

/** Callback function used to parse a single xml element. Used by
    XML_SetElementHandler().

    Store result in the #conf variable. */
static void start(void *data __attribute__ ((__unused__)),
		  const char *el, const char **attr) {

  // parse get_version
  if (strcmp(el, "get_version") == 0) {
    if (strcmp(attr[0], "value") == 0)
      conf.get_version = attr[1];
  }

  // parse prime command
  if (strcmp(el, "prime") == 0) {
    if (strcmp(attr[0], "command") == 0)
      conf.prime_command = attr[1];
  }

  // parse simple flags
  if (strcmp(el, "flag") == 0) {
    if (strcmp(attr[0], "type") == 0 && strcmp(attr[1], "simple") == 0 &&
	strcmp(attr[2], "value") == 0)
      conf.flags.push_back(new simple_t(attr[3]));

    if (strcmp(attr[0], "type") == 0 && strcmp(attr[1], "enum") == 0 &&
	strcmp(attr[2], "value") == 0)
      conf.flags.push_back(new enum_t(attr[3]));
  }

#if DEBUG
  for (int i = 0; i < Depth; i++)
    printf("  ");

  printf("%s", el);

  for (int i = 0; attr[i] != NULL; i += 2) {
    printf(" %s='%s'", attr[i], attr[i + 1]);
  }

  printf("\n");
  Depth++;
#endif
}

/** Callback function which is called at the end of a single xml
    element. Used by XML_SetElementHandler(). */
static void end(void *data __attribute__ ((__unused__)),
		const char *el __attribute__ ((__unused__))) {
#if DEBUG
  Depth--;
#endif
}

/** Parse the XML configuration file from a memory buffer. */
static void parse(str_array conf_file_buf) {

  XML_Parser parser = XML_ParserCreate(NULL);
  assert(parser != NULL);

  XML_SetElementHandler(parser, start, end);

  if (! XML_Parse(parser, conf_file_buf.str, conf_file_buf.len, true)) {
    fprintf(stderr, "Parse error at line %ld:\n%s\n",
	    XML_GetCurrentLineNumber(parser),
	    XML_ErrorString(XML_GetErrorCode(parser)));
    exit(EXIT_FAILURE);
  }

#if DEBUG
  printf("prime command: %s\n",	 conf.prime_command.c_str());
  printf("baselines:\n");
  for (conf_t::baselines_t::const_iterator i = conf.baselines.begin();
       i < conf.baselines.end(); ++i)
    printf("%s\n", i->c_str());
  printf("flags:\n");
  for (conf_t::flags_t::const_iterator i = conf.flags.begin();
       i < conf.flags.end(); ++i) {
    printf("%3ld: ", i-conf.flags.begin());
    for (unsigned j = 1; j < (*i)->size(); ++j)
      printf("%s ", (*i)->get_flag(j).c_str());
    printf("\n");
  }
#endif
}

/** Read the configuration file and parse it into #conf. */

void read_conf() {
  str_array conf_file_buf = file_read(config_file);
  parse(conf_file_buf);
}
