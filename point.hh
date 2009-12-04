#ifndef POINT_HH
#define POINT_HH

#include <string>
using std::string;
#include <vector>
using std::vector;

// point_t

struct point_t {
  explicit point_t();
  explicit point_t(string flags_str);
  bool operator==(point_t const& p) const;
  bool operator!=(point_t const& p) const;
  bool operator<(point_t const& p) const;
  string str() const;
  void set_rand();
  typedef vector<unsigned char> val_t;
  val_t val;
};

// obj_t

struct obj_t {
  explicit obj_t();
  explicit obj_t(int64_t val);
  explicit obj_t(string str);
  obj_t operator-(obj_t const& obj) const;
  bool operator==(obj_t const& obj) const;
  bool operator<(obj_t const& obj) const;
  bool operator>(obj_t const& obj) const;
  string str() const;
private:
  explicit obj_t(bool inf, int64_t val);
  bool inf; // mark infinite values
  int64_t val; // value (sign for infinite values)
};

#endif // POINT_HH
