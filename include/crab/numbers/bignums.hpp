#pragma once

#include <boost/functional/hash.hpp>
#include <crab/support/os.hpp>
#include <cstdint>
#include <gmp.h>

// TODO: replace ikos with crab namespace. This class has nothing to
// do with the ikos one. Kept for now for compatibility issues with
// some clients.
namespace ikos {

enum NumDiscriminant {
    Z_Number,
    FP_Number,
    Q_Number
};

class z_number {
  friend class q_number;
  friend class u_number;

private:
  mpz_t _n;
  double _f = 0;
  NumDiscriminant discr = Z_Number;

  bool fits_sint() const;
  bool fits_slong() const;

  // used only by q_number
  static z_number from_mpz_t(mpz_t n);
  static z_number from_mpz_srcptr(mpz_srcptr n);
  
public:

  z_number();
  z_number(NumDiscriminant d) : discr(d) {}
  z_number(double f, NumDiscriminant d) : _f(f), discr(d) {}
//  explicit z_number(double f) : _f(f), discr(FP_Number) {}
  z_number(int64_t n);
  z_number(const std::string &s, unsigned base = 10);
  static z_number from_uint64(uint64_t n);
  /* Return a z_number from num_words words of binary data. If order
   is true then most significant word first, otherwise the least
   significant first. The number is treated as unsigned so sign must
   be treated separately. */  
  static z_number from_raw_data(const uint64_t*data, size_t num_words,
				bool order = false);
  /* Return an array of words from this. If the number is positive
   then sign is true. If order is true then most significant word
   first, otherwise the least significant first */  
  uint64_t *to_raw_data(size_t &num_words, bool &sign, bool order = false);
  
  ~z_number();
  
  z_number(const z_number &o);
  z_number(z_number &&o);
  z_number &operator=(const z_number &o);
  z_number &operator=(z_number &&o);

  // overloaded typecast operators
  explicit operator int64_t() const;
  
  std::string get_str(unsigned base = 10) const;

  // do not use it: to be removed
  mpz_ptr get_mpz_t() {
    if (discr == FP_Number) { assert(0); }
    return _n;
  }

  double get_double() const {
    if (discr == FP_Number) { return _f; }
    if (discr == Z_Number)  { return mpz_get_d(_n); }
  }

  std::size_t hash() const;

  bool fits_int64() const;

  z_number operator+(z_number x) const;

  z_number operator*(z_number x) const;

  z_number operator-(z_number x) const;

  z_number operator-() const;

  // signed division
  z_number operator/(z_number x) const;

  // signed remainder
  z_number operator%(z_number x) const;

  z_number &operator+=(z_number x);

  z_number &operator*=(z_number x);

  z_number &operator-=(z_number x);

  z_number &operator/=(z_number x);

  z_number &operator%=(z_number x);

  z_number &operator--();

  z_number &operator++();

  z_number operator++(int);

  z_number operator--(int);

  bool operator==(z_number x) const;

  bool operator!=(z_number x) const;

  bool operator<(z_number x) const;

  bool operator<=(z_number x) const;

  bool operator>(z_number x) const;

  bool operator>=(z_number x) const;

  // bitwise-and
  z_number operator&(z_number x) const;

  // bitwise-or
  z_number operator|(z_number x) const;

  // bitwise-xor
  z_number operator^(z_number x) const;

  // left shift
  z_number operator<<(z_number x) const;

  // arithmetic right shift
  z_number operator>>(z_number x) const;

  z_number fill_ones() const;

  void write(crab::crab_os &o) const;

}; // class z_number

class fp_number {
  friend class q_number;
  friend class z_number;

private:
  double _n;

public:
  fp_number() = default;
  fp_number(double n) : _n(n) {}
  ~fp_number() = default;

  fp_number(const fp_number &o) = default;
  fp_number(fp_number &&o) = default;
  fp_number &operator=(const fp_number &o) = default;
  fp_number &operator=(fp_number &&o) = default;

  fp_number(const std::string &s, unsigned base = 10) {
      _n = std::stod(s);
  }

  double get_double() const { return _n; };

  fp_number operator+(fp_number x) const { return this->_n + x._n; }

  fp_number operator*(fp_number x) const { return this->_n * x._n; }

  fp_number operator-(fp_number x) const { return this->_n - x._n; }

  fp_number operator-() const { return -this->_n; }

  fp_number operator/(fp_number x) const { return this->_n / x._n; }

  fp_number &operator+=(fp_number x) { return *this = *this + x; }

  fp_number &operator*=(fp_number x) { return *this = *this * x; }

  fp_number &operator-=(fp_number x) { return *this = *this - x; }

  fp_number &operator/=(fp_number x) { return *this = *this / x; }

  fp_number &operator--() { return *this = *this - 1.0; }

  fp_number &operator++() { return *this = *this + 1.0; }

  bool operator==(fp_number x) const { return this->_n == x._n; }

  bool operator!=(fp_number x) const { return this->_n != x._n; }

  bool operator<(fp_number x) const { return this->_n < x._n; }

  bool operator<=(fp_number x) const { return this->_n <= x._n; }

  bool operator>(fp_number x) const { return this->_n > x._n; }

  bool operator>=(fp_number x) const { return this->_n >= x._n; }

  // left shift
  fp_number operator<<(fp_number x) const {
    assert(false); // Left-shift not supported for fp_number
    return x;
  }

  void write(crab::crab_os &o) const {
    o << _n;
  }

}; // class fp_number

class q_number {

private:
  mpq_t _n;

public:
  q_number();
  q_number(double n);

  q_number(const std::string &s, unsigned base = 10);
  q_number(const z_number &n);
  q_number(const fp_number &n);
  q_number(const z_number &n, const z_number &d);

  static q_number from_mpq_t(mpq_t n);
  static q_number from_mpz_t(mpz_t n);
  static q_number from_mpq_srcptr(mpq_srcptr q);

  q_number(const q_number &o);
  q_number(q_number &&o);
  q_number &operator=(const q_number &o);
  q_number &operator=(q_number &&o);

  ~q_number();

  mpq_srcptr get_mpq_t() const { return _n; }

  mpq_ptr get_mpq_t() { return _n; }

  double get_double() const;

  std::string get_str(unsigned base = 10) const;

  std::size_t hash() const;

  q_number operator+(q_number x) const;

  q_number operator*(q_number x) const;

  q_number operator-(q_number x) const;

  q_number operator-() const;

  q_number operator/(q_number x) const;

  q_number &operator+=(q_number x);

  q_number &operator*=(q_number x);

  q_number &operator-=(q_number x);

  q_number &operator/=(q_number x);

  q_number &operator--();

  q_number &operator++();

  q_number operator--(int);

  q_number operator++(int);

  bool operator==(q_number x) const;

  bool operator!=(q_number x) const;

  bool operator<(q_number x) const;

  bool operator<=(q_number x) const;

  bool operator>(q_number x) const;

  bool operator>=(q_number x) const;

  // left shift
  q_number operator<<(q_number x) const;
  
  z_number numerator() const;

  z_number denominator() const;

  z_number round_to_upper() const;

  z_number round_to_lower() const;

  void write(crab::crab_os &o) const;

}; // class q_number

/* Experimental. DO NOT USE! */
class u_number {

    private:
    z_number  _z;
    fp_number _f;
    NumDiscriminant discr = Z_Number; // By default it's integer

    bool fits_sint() const;
    bool fits_slong() const;

    // used only by q_number
    static u_number from_mpz_t(mpz_t n);
    static u_number from_mpz_srcptr(mpz_srcptr n);

public:
    u_number();
    u_number(z_number z);
    u_number(fp_number z);
    u_number(int64_t n);
    u_number(const std::string &s, unsigned base = 10);
    static u_number from_uint64(uint64_t n);
    /* Return a u_number from num_words words of binary data. If order
     is true then most significant word first, otherwise the least
     significant first. The number is treated as unsigned so sign must
     be treated separately. */
    static u_number from_raw_data(const uint64_t*data, size_t num_words, bool order = false);
    /* Return an array of words from this. If the number is positive
     then sign is true. If order is true then most significant word
     first, otherwise the least significant first */
    uint64_t *to_raw_data(size_t &num_words, bool &sign, bool order = false);

    ~u_number();

    u_number(const u_number &o);
    u_number(u_number &&o);
    u_number &operator=(const u_number &o);
    u_number &operator=(u_number &&o);

    // overloaded typecast operators
    explicit operator int64_t() const;

    std::string get_str(unsigned base = 10) const;

    // do not use it: to be removed
    mpz_ptr get_mpz_t() {
        if (discr == Z_Number) {
            return _z.get_mpz_t();
        }
        else {
            assert(0);
            return _z.get_mpz_t();
        }
    }

    double get_double() {
        if (discr == Z_Number) {
            return _z.get_double();
        }
        else {
            assert(0);
            return _f.get_double();
        }
    }

    std::size_t hash() const;

    bool fits_int64() const;

    u_number operator+(u_number x) const;

    u_number operator*(u_number x) const;

    u_number operator-(u_number x) const;

    u_number operator-() const;

    // signed division
    u_number operator/(u_number x) const;

    // signed remainder
    u_number operator%(u_number x) const;

    u_number &operator+=(u_number x);

    u_number &operator*=(u_number x);

    u_number &operator-=(u_number x);

    u_number &operator/=(u_number x);

    u_number &operator%=(u_number x);

    u_number &operator--();

    u_number &operator++();

    u_number operator++(int);

    u_number operator--(int);

    bool operator==(u_number x) const;

    bool operator!=(u_number x) const;

    bool operator<(u_number x) const;

    bool operator<=(u_number x) const;

    bool operator>(u_number x) const;

    bool operator>=(u_number x) const;

    // bitwise-and
    u_number operator&(u_number x) const;

    // bitwise-or
    u_number operator|(u_number x) const;

    // bitwise-xor
    u_number operator^(u_number x) const;

    // left shift
    u_number operator<<(u_number x) const;

    // arithmetic right shift
    u_number operator>>(u_number x) const;

    u_number fill_ones() const;

    void write(crab::crab_os &o) const;

}; // class u_number (universal numer)

inline crab::crab_os &operator<<(crab::crab_os &o, const z_number &z) {
  z.write(o);
  return o;
}

inline crab::crab_os &operator<<(crab::crab_os &o, const fp_number &z) {
  z.write(o);
  return o;
}

inline crab::crab_os &operator<<(crab::crab_os &o, const q_number &q) {
  q.write(o);
  return o;
}

inline crab::crab_os &operator<<(crab::crab_os &o, const u_number &u) {
    u.write(o);
    return o;
}

/** for boost::hash_combine **/
inline std::size_t hash_value(const z_number &z) { return z.hash(); }

inline std::size_t hash_value(const q_number &q) { return q.hash(); }

inline std::size_t hash_value(const u_number &u) { return u.hash(); }
} // namespace ikos

/** for specializations of std::hash **/
namespace std {
template <> struct hash<ikos::z_number> {
  size_t operator()(const ikos::z_number &z) const { return z.hash(); }
};

template <> struct hash<ikos::q_number> {
  size_t operator()(const ikos::q_number &q) const { return q.hash(); }
};

template <> struct hash<ikos::u_number> {
    size_t operator()(const ikos::u_number &u) const { return u.hash(); }
};
} // namespace std
