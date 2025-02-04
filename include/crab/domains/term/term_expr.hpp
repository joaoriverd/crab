#pragma once

#include <forward_list>
#include <map>
#include <memory>
#include <vector>

#include <boost/container/flat_set.hpp>
#include <boost/optional.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"

namespace crab {

namespace domains {

namespace term {
using term_id = int;
using var_id = int;

enum term_kind { TERM_CONST, TERM_VAR, TERM_APP };

template <class Num, class Ftor> class term {
public:
  using term_t = term<Num, Ftor>;
  using term_ptr = std::shared_ptr<term_t>;

  virtual ~term() {}
  virtual term_kind kind(void) const = 0;
  bool operator<(const term_t &other) const;
  virtual void write(crab_os &o) const = 0;

  int depth;
};

template <class Num, class Ftor>
inline crab::crab_os &operator<<(crab::crab_os &o, const term<Num, Ftor> &t) {
  t.write(o);
  return o;
}

template <class Num, class Ftor> class term_ref {
public:
  using term_t = term<Num, Ftor>;
  using term_ptr = std::shared_ptr<term_t>;

  term_ref(term_ptr _p) : p(_p) {}
  term_ref(term_t *_p) : p(_p) {}
  bool operator<(const term_ref &other) const { return *p < *(other.p); }
  
  term_ptr p;
};

// Dispatch for different kinds of terms.
template <class Num, class Ftor> class const_term : public term<Num, Ftor> {
public:
  const_term(Num _val) : val(_val) {}
  virtual term_kind kind(void) const override { return TERM_CONST; }
  virtual void write(crab_os &o) const override { o << "c(" << val << ")"; }
  
  Num val;
};

template <class Num, class Ftor> class var_term : public term<Num, Ftor> {
public:
  var_term(var_id _var) : var(_var) {}
  virtual term_kind kind(void) const override { return TERM_VAR; }
  virtual void write(crab_os &o) const override { o << "v(" << var << ")"; }
  
  var_id var;
};

template <class Num, class Ftor> class ftor_term : public term<Num, Ftor> {
public:
  ftor_term(Ftor _f, const std::vector<term_id> &_args) : ftor(_f), args(_args) {}
  virtual term_kind kind(void) const override { return TERM_APP; }
  virtual void write(crab_os &o) const override {
    o << ftor;
    if (!args.empty()) {
      o << "(";
      bool first = true;
      for (term_id c : args) {
	if (first)
	  first = false;
	else
	  o << ", ";
	o << c;
      }
      o << ")";
    }
  }

  Ftor ftor;
  std::vector<term_id> args;
};

// These functions are super-unsafe.
template <class Num, class Ftor>
var_id term_var(const term<Num, Ftor> *term) {
  return static_cast<const var_term<Num, Ftor> *>(term)->var;
}
template <class Num, class Ftor>
Num term_const(const term<Num, Ftor> *term) {
  return static_cast<const const_term<Num, Ftor> *>(term)->val;
}
template <class Num, class Ftor>
Ftor term_ftor(const term<Num, Ftor> *term) {
  return static_cast<const ftor_term<Num, Ftor> *>(term)->ftor;
}

template <class Num, class Ftor>
const std::vector<term_id> &term_args(const term<Num, Ftor> *term) {
  return static_cast<const ftor_term<Num, Ftor> *>(term)->args;
}

// Term ordering
template <class Num, class Ftor>
bool term<Num, Ftor>::operator<(const term_t &other) const {
  if (kind() != other.kind())
    return kind() < other.kind();

  switch (kind()) {
  case TERM_CONST: {
    return term_const(this) < term_const(&other);
  } break;
  case TERM_VAR: {
    return term_var(this) < term_var(&other);
  }
  case TERM_APP: {
    if (term_ftor(this) != term_ftor(&other))
      return term_ftor(this) < term_ftor(&other);

    const std::vector<int> &xargs(term_args(this));
    const std::vector<int> &yargs(term_args(&other));

    if (xargs.size() != yargs.size())
      return xargs.size() < yargs.size();

    for (unsigned int ii = 0; ii < xargs.size(); ii++) {
      if (xargs[ii] != yargs[ii])
        return xargs[ii] < yargs[ii];
    }
    return false;
  }
  default:
    CRAB_ERROR("Unsupported term kind.");
  }
}

template <typename TermTable> class congruence_closure_solver;

template <class Num, class Ftor> class term_table {
public:
  using term_table_t = term_table<Num, Ftor>;
  using term_t = term<Num, Ftor>;
  using term_ptr = std::shared_ptr<term_t>;
  using term_ref_t = term_ref<Num, Ftor>;
  using term_id_t = term_id;

  using const_term_t = const_term<Num, Ftor>;
  using var_term_t = var_term<Num, Ftor>;
  using ftor_term_t = ftor_term<Num, Ftor>;

  // For establishing a mapping between tables
  using term_map_t = std::map<term_id, term_id>;
  using gener_map_t = std::map<std::pair<term_id, term_id>, term_id>;

private:
  
  term_id fresh_term(term_ref_t ref) {
    if (m_free_terms.size() > 0) {
      term_id t = m_free_terms.back();
      m_free_terms.pop_back();
      m_terms[t] = ref;
      m_parents[t].clear();
      m_depth[t] = 0;
      m_ref_count[t] = 0;
      return t;
    } else {
      term_id t = m_terms.size();
      m_terms.push_back(ref);
      m_ref_count.push_back(0);
      m_parents.push_back(std::forward_list<term_id>());
      m_depth.push_back(0);
      return t;
    }
  }

  term_id add_term(term_ref_t ref) {
    auto it = m_map.find(ref);
    if (it != m_map.end()) {
      return (*it).second;
    } else {
      term_id id = fresh_term(ref);
      m_map[ref] = id;
      if (ref.p.get()->kind() == TERM_APP) {
        unsigned int c_depth = 0;
        for (term_id c : term_args(ref.p.get())) {
          assert(c < m_ref_count.size());
          m_ref_count[c] += 1;
          // do not keep order between parents
          m_parents[c].push_front(id);
          c_depth = std::max(c_depth, m_depth[c]);
        }
        m_depth[id] = 1 + c_depth;
      }
      /* Not true, as we're garbage collecting terms */
      // assert(m_map.size() == id+1);
      return id;
    }
  }

  int m_free_var;
  std::map<term_ref_t, term_id> m_map;
  std::vector<term_ref_t> m_terms;
  std::vector<std::forward_list<term_id_t>> m_parents;
  // JN: I tested using vec instead of std::vector and performance
  // didn't improve much.
  std::vector<unsigned int> m_ref_count;
  std::vector<unsigned int> m_depth;
  std::vector<term_id> m_free_terms;
  
public:
  
  term_table(void) : m_free_var(0) {}
  term_table(const term_table_t &o) = default;
  term_table_t &operator=(const term_table_t &o) = default;
  term_table(term_table_t &&o) = default;
  term_table_t &operator=(term_table_t &&o) = default;

  term_id make_const(const Num &n) {
    term_ref_t ref(new const_term_t(n));
    return add_term(ref);
  };
  boost::optional<term_id> find_const(const Num &n) {
    term_ref_t ref(new const_term_t(n));
    return find_term(ref);
  }

  term_id make_var(var_id n) {
    m_free_var = std::max(m_free_var, n + 1);
    term_ref_t ref(new var_term_t(n));
    return add_term(ref);
  };

  term_id fresh_var(void) {
    var_id v = m_free_var++;
    return make_var(v);
  }

  template <typename T, typename... Types>
  void collect_args(std::vector<T> &vec, T &x, Types... rest) {
    vec.push_back(x);
    collect_args(vec, rest...);
  }

  template <typename T> void collect_args(std::vector<T> &vec) {}

  term_id apply_ftor(const Ftor &f, const std::vector<term_id> &ids) {
    term_ref_t ref(new ftor_term_t(f, ids));
    return add_term(ref);
  }

  template <typename... Types>
  term_id apply_ftor(const Ftor &f, Types... args) {
    std::vector<term_id> ids;
    collect_args(ids, args...);
    return apply_ftor(f, ids);
  }

  boost::optional<term_id> find_ftor(Ftor &f, const std::vector<term_id> &ids) {
    term_ref_t ref(new ftor_term_t(f, ids));
    return find_term(ref);
  }

  template <typename... Types>
  boost::optional<term_id> find_ftor(Ftor &f, Types... args) {
    std::vector<term_id> ids;
    collect_args(ids, args...);
    return find_ftor(f, ids);
  }

  term_t *get_term_ptr(term_id t) { return m_terms[t].p.get(); }

  const term_t *get_term_ptr(term_id t) const { return m_terms[t].p.get(); }

  void add_ref(term_id t) { m_ref_count[t]++; }

  void deref(term_id t, std::vector<term_id> &forgotten) {
    assert(m_ref_count[t]);
    m_ref_count[t]--;
    if (!m_ref_count[t]) {
      forgotten.push_back(t);
      m_free_terms.push_back(t);
      term_ref_t ref(m_terms[t]);
      m_map.erase(ref);
      if (ref.p.get()->kind() == TERM_APP) {
        for (term_id c : term_args(ref.p.get())) {
          // remove t as parent of c before going down child
          auto &c_parents = parents(c);
          c_parents.remove(t);
          deref(c, forgotten);
        }
      }
    }
  }

  // Check if a tx is a generalization of ty, given an existing context.
  bool map_leq(term_table_t &y, term_id tx, term_id ty, term_map_t &map) const {
    auto it = map.find(ty);
    if (it != map.end())
      return (*it).second == tx;

    term_ref_t ry(y.m_terms[ty]);
    term_ref_t rx(m_terms[tx]);
    switch (ry.p.get()->kind()) {
    case TERM_CONST: {
      if (rx.p.get()->kind() != TERM_CONST ||
          term_const(rx.p.get()) != term_const(ry.p.get()))
        return false;
    } break;
    case TERM_APP: {
      if (rx.p.get()->kind() != TERM_APP ||
          term_ftor(rx.p.get()) != term_ftor(ry.p.get()))
        return false;

      const std::vector<int> &xargs(term_args(rx.p.get()));
      const std::vector<int> &yargs(term_args(ry.p.get()));

      if (xargs.size() != yargs.size())
        return false;

      for (unsigned int ii = 0; ii < xargs.size(); ii++) {
        if (!map_leq(y, xargs[ii], yargs[ii], map))
          return false;
      }
    } break;
    case TERM_VAR:
      break;
    }
    map[ty] = tx;
    return true;
  }

  term_id_t generalize(term_table_t &y, term_id tx, term_id ty,
                       term_table_t &out, gener_map_t &g_map) const {
    auto txy = std::make_pair(tx, ty);
    auto it = g_map.find(txy);
    if (it != g_map.end()) {
      return (*it).second;
    } else {
      // Haven't found this pair yet.
      // Generalize the arguments.
      auto px(m_terms[tx].p.get());
      auto py(y.m_terms[ty].p.get());

      term_kind kx = px->kind();
      term_kind ky = py->kind();

      // If either is a variable, we just create a fresh variable
      term_id_t ret;
      do {
        if (kx == TERM_VAR || ky == TERM_VAR || kx != ky) {
          ret = out.fresh_var();
          break;
        }

        if (kx == TERM_CONST) {
          if (term_const(px) != term_const(py)) {
            ret = out.fresh_var();
          } else {
            ret = out.make_const(term_const(px));
          }
        } else {
          assert(kx == TERM_APP);
          if (term_ftor(px) != term_ftor(py)) {
            ret = out.fresh_var();
            break;
          }

          const std::vector<term_id> &xargs(term_args(px));
          const std::vector<term_id> &yargs(term_args(py));
          if (xargs.size() != yargs.size()) {
            ret = out.fresh_var();
            break;
          }

          std::vector<term_id> xyargs;
          for (unsigned int ii = 0; ii < xargs.size(); ii++)
            xyargs.push_back(generalize(y, xargs[ii], yargs[ii], out, g_map));
          ret = out.apply_ftor(term_ftor(px), xyargs);
        }
      } while (0);

      g_map[txy] = ret;
      return ret;
    }
  }

  // Copy term x from table tx
  term_id_t copy_term(const term_table_t &tx, term_id x, term_map_t &ren_map) {
    auto it = ren_map.find(x);
    if (it != ren_map.end()) {
      return (*it).second;
    } else {
      auto px(tx.m_terms[x].p.get());
      term_kind kx = px->kind();
      term_id_t ret;
      if (kx == TERM_VAR) {
        ret = fresh_var();
      } else if (kx == TERM_CONST) {
        ret = make_const(term_const(px));
      } else {
        assert(kx == TERM_APP);
        const std::vector<term_id> &xargs(term_args(px));
        std::vector<term_id> yargs;
        for (unsigned int ii = 0; ii < xargs.size(); ii++)
          yargs.push_back(copy_term(tx, xargs[ii], ren_map));
        ret = apply_ftor(term_ftor(px), yargs);
      }

      ren_map[x] = ret;
      return ret;
    }
  }

  std::forward_list<term_id_t> &parents(term_id_t id) { return m_parents[id]; }

  void write(crab_os &o) const {
    bool first = true;
    for (unsigned int ti = 0; ti < m_terms.size(); ti++) {
      if (first)
        first = false;
      else
        o << ", ";
      term_t *p(m_terms[ti].p.get());
      o << ti << " -> " << *p;
    }
  }

  int size() const { return m_terms.size(); }

  int depth(term_id t) { return m_depth[t]; }

  boost::optional<term_id> find_term(term_ref_t ref) const {
    auto it = m_map.find(ref);
    if (it != m_map.end())
      return boost::optional<term_id>(it->second);
    else
      return boost::optional<term_id>();
  }  
};

template <class Num, class Ftor>
inline crab::crab_os &operator<<(crab::crab_os &o,
                                 const term_table<Num, Ftor> &t) {
  t.write(o);
  return o;
}

template <typename TermTable> class congruence_closure_solver {

  using this_type = congruence_closure_solver<TermTable>;

public:
  using term_id_t = typename TermTable::term_id_t;
  using term_t = typename TermTable::term_t;
  using term_set_t = boost::container::flat_set<term_id_t>;
  using ccpar_map_t = std::map<term_id_t, term_set_t>;
  using find_map_t = std::map<term_id_t, term_id_t>;
  using equation_t = std::pair<term_id_t, term_id_t>;
  using member_map_t = std::map<term_id_t, std::vector<term_id_t>>;

private:
  TermTable *_ttbl;
  ccpar_map_t _ccpar_map;
  find_map_t _find_map;
  std::vector<equation_t> _eqs;
  term_set_t _terms;
  member_map_t _members;

public:
  congruence_closure_solver(TermTable *ttbl) : _ttbl(ttbl) {}

  congruence_closure_solver(const this_type &o) = delete;

  this_type &operator=(const this_type &o) = delete;

  void operator+=(equation_t eq) {
    _terms.insert(eq.first);
    _terms.insert(eq.second);
    _eqs.push_back(eq);
  }

  void run() {
    for (auto eq : _eqs) {
      term_id_t x = eq.first;
      term_id_t y = eq.second;
      merge(x, y);
    }
  }

  void run(const std::vector<equation_t> &eqs) {
    for (auto e : eqs) {
      *this += e;
    }
    run();
  }

  // return the equivalence class associated with t
  term_id_t get_class(term_id_t t) { return find(t); }

  // return the members of an equivalence class of k
  std::vector<term_id_t> &get_members(term_id_t t) {
    auto it = _members.find(t);
    if (it != _members.end()) {
      return it->second;
    } else {
      t = find(t); // find representative of the t's class
      std::vector<term_id_t> members;
      for (auto x : _terms) {
        if (find(x) == t /*find (t)*/)
          members.push_back(x);
      }
      auto res = _members.insert(std::make_pair(t, members));
      return (res.first)->second;
    }
  }

  // return the size of the equivalence class of k
  size_t get_size(term_id_t t) { return get_members(t).size(); }

  void write(crab_os &o) const {
    for (auto t1 : _terms) {
      o << "t" << t1 << " --> {";
      for (auto t2 : _terms) {
        if (find(t1) == find(t2))
          o << "t" << t2 << ";";
      }
      o << "}\n";
    }
  }

  std::forward_list<term_id_t> &get_parents(term_id_t t) {
    return _ttbl->parents(t);
  }

private:
  term_set_t &get_ccpar(term_id_t t) {
    auto it = _ccpar_map.find(t);
    if (it == _ccpar_map.end()) {
      auto res = _ccpar_map.insert(std::make_pair(t, term_set_t()));
      return (res.first)->second;
    } else {
      return it->second;
    }
  }

  // return the representative of i
  term_id_t find(term_id_t t) {
    auto it = _find_map.find(t);
    if (it == _find_map.end()) {
      _find_map.insert(std::make_pair(t, t));
      return t;
    }

    if (it->second == t) {
      return it->second;
    } else {
      return find(it->second);
    }
  }

  // i2 is the new representative
  void do_union(term_id_t t1, term_id_t t2) {
    _find_map[t1] = _find_map[t2];

    term_set_t &ccpar1 = get_ccpar(t1);
    term_set_t &ccpar2 = get_ccpar(t2);
    ccpar2.insert(ccpar1.begin(), ccpar1.end());

    term_t *t1_ptr = _ttbl->get_term_ptr(t1);
    assert(t1_ptr);
    if (t1_ptr->kind() == TERM_APP) {
      auto &t1_parents = get_parents(t1);
      ccpar2.insert(t1_parents.begin(), t1_parents.end());
    }

    term_t *t2_ptr = _ttbl->get_term_ptr(t2);
    assert(t2_ptr);
    if (t2_ptr->kind() == TERM_APP) {
      auto &t2_parents = get_parents(t2);
      ccpar2.insert(t2_parents.begin(), t2_parents.end());
    }
    ccpar1.clear();
  }

  // pre: t1 and t2 are TERM_APP
  bool is_congruent(term_id_t t1, term_id_t t2) {

    term_t *t1_ptr = _ttbl->get_term_ptr(t1);
    term_t *t2_ptr = _ttbl->get_term_ptr(t2);

    assert(t1_ptr->kind() == TERM_APP);
    assert(t2_ptr->kind() == TERM_APP);

    if (term_ftor(t1_ptr) != term_ftor(t2_ptr))
      return false;

    const std::vector<term_id_t> &xargs(term_args(t1_ptr));
    const std::vector<term_id_t> &yargs(term_args(t2_ptr));

    if (xargs.size() != yargs.size())
      return false;

    for (unsigned i = 0; i < xargs.size(); i++) {
      if (find(xargs[i]) != find(yargs[i]))
        return false;
    }
    return true;
  }

  void merge(term_id_t t1, term_id_t t2) {

    if (find(t1) == find(t2))
      return;

    do_union(t1, t2);
    auto &ps1 = get_ccpar(t1);
    auto &ps2 = get_ccpar(t2);
    for (auto p1 : ps1) {
      for (auto p2 : ps2) {
        if ((find(p1) != find(p2)) && is_congruent(p1, p2))
          merge(p1, p2);
      }
    }
  }
}; // end congruence_closure_solver

} // end namespace term
} // namespace domains
} // end namespace crab
#pragma GCC diagnostic pop
