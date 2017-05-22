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
  using val_t = vector<unsigned char>;
  val_t val;
};

// obj_t

struct obj_t {
  explicit obj_t();
  explicit obj_t(int64_t v);
  explicit obj_t(const string& str);
  obj_t operator-(obj_t const& obj) const;
  bool operator==(obj_t const& obj) const;
  bool operator<(obj_t const& obj) const;
  bool operator>(obj_t const& obj) const;
  string str() const;
private:
  explicit obj_t(bool i, int64_t v);
  bool inf{}; // mark infinite values
  int64_t val{}; // value (sign for infinite values)
};

#endif // POINT_HH
