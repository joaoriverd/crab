#include "../common.hpp"
#include "../program_options.hpp"

using namespace std;
using namespace crab::cfg;
using namespace crab::cfg_impl;
using namespace crab::domain_impl;

#define FP_PROGRAM

#if defined(FP_PROGRAM)
#define t_cfg_t               fp_cfg_t
#define t_var                 fp_var
#define t_oct_elina_domain_t  fp_oct_elina_domain_t
#define var_type              crab::FP_TYPE
#define t_number              ikos::fp_number
#elif defined(REAL_PROGRAM)
#define t_cfg_t               q_cfg_t
#define t_var                 q_var
#define t_oct_elina_domain_t  q_oct_elina_domain_t
#define var_type              crab::REAL_TYPE
#define t_number              ikos::q_number
#else
#define t_cfg_t               z_cfg_t
#define t_var                 z_var
#define t_oct_elina_domain_t  z_oct_elina_domain_t
#define var_type              crab::INT_TYPE
#define t_number              ikos::z_number
#endif

/* Example of how to build a CFG */
t_cfg_t *prog(variable_factory_t &vfac) {

  /*
    k := 2147483648;
    i := 0;
    while (i <= 99) {
      i += (* ? 1: 2);
    }
   */
  // Defining program variables
  t_var i(vfac["i"], crab::INT_TYPE, 32);
  t_var k(vfac["k"], crab::INT_TYPE, 32);
  t_var nd(vfac["nd"], crab::INT_TYPE, 32);
  t_var inc(vfac["inc"], crab::INT_TYPE, 32);
  // entry and exit block
  auto cfg = new t_cfg_t("x0", "ret");
  // adding blocks
  auto &x0 = cfg->insert("x0");
  auto &x1 = cfg->insert("x1");
  auto &x2 = cfg->insert("x2");
  auto &x3 = cfg->insert("x3");
  auto &entry = cfg->insert("entry");
  auto &bb1 = cfg->insert("bb1");
  auto &bb1_t = cfg->insert("bb1_t");
  auto &bb1_f = cfg->insert("bb1_f");
  auto &bb2 = cfg->insert("bb2");
  auto &ret = cfg->insert("ret");
  // adding control flow
  x0 >> x1;
  x1 >> x2;
  x2 >> x3;
  x3 >> entry;
  entry >> bb1;
  bb1 >> bb1_t;
  bb1 >> bb1_f;
  bb1_t >> bb2;
  bb2 >> bb1;
  bb1_f >> ret;
  // adding statements
  x0.assign(k, 2147483648);
  entry.assign(i, 0);
  bb1_t.assume(i <= 99);
  bb1_f.assume(i >= 100);
  bb2.havoc(nd);
  bb2.select(inc, nd, 1, 2);
  bb2.add(i, i, inc);

  return cfg;
}

t_cfg_t *prog_2(variable_factory_t &vfac) {

  // Definining program variables
  t_var x(vfac["x"], var_type);
  t_var y(vfac["y"], var_type);
  // entry and exit block
  auto *cfg = new t_cfg_t("entry", "bb1");
  // adding blocks
  auto &entry = cfg->insert("entry");
  auto &bb1 = cfg->insert("bb1");
  // adding control flow
  entry >> bb1;
  bb1 >> bb1;
  // adding statements
  entry.assign(x, t_number(0.0));
  entry.assign(y, t_number(0.5));
  bb1.add(x, x, y);
  bb1.mul(y, y, t_number(0.5));

  return cfg;
}

t_cfg_t *prog_jr(variable_factory_t &vfac) {
  /*
    int x = 0;
    while (x <= 99) {
      x++;
    }
  */
    // Defining program variables
  t_var x(vfac["x"], var_type, 32);
  // entry and exit block
  auto cfg = new t_cfg_t("entry", "ret");
  // adding blocks
  auto &entry = cfg->insert("entry");
  auto &bb1 = cfg->insert("bb1");
  auto &bb1_t = cfg->insert("bb1_t");
  auto &bb1_f = cfg->insert("bb1_f");
  auto &ret = cfg->insert("ret");
  // adding control flow
  entry >> bb1;
  bb1 >> bb1_t;
  bb1 >> bb1_f;
  bb1_t >> bb1;
  bb1_f >> ret;
  // adding statements
  entry.assign(x, t_number(0.9));
  bb1_t.assume(x <= t_number(99.1));
  bb1_t.add(x, x, t_number(0.1));
  bb1_f.assume(x > t_number(99.1));

  return cfg;
}

/* Example of how to infer invariants from the above CFG */
int main(int argc, char **argv) {
  bool stats_enabled = false;
  if (!crab_tests::parse_user_options(argc, argv, stats_enabled)) {
    return 0;
  }

  variable_factory_t vfac;
  t_cfg_t *cfg = prog_2(vfac);
  cfg->simplify(); // this is optional
  crab::outs() << *cfg << "\n";

  {
    t_oct_elina_domain_t init;
    run(cfg, cfg->entry(), init, false, 2, 2, 20, stats_enabled);
  }

//  {
//    z_interval_domain_t init;
//    run(cfg, cfg->entry(), init, false, 1, 2, 20, stats_enabled);
//  }
//  {
//    z_dbm_domain_t init;
//    run(cfg, cfg->entry(), init, false, 1, 2, 20, stats_enabled);
//  }
//  {
//    z_sdbm_domain_t init;
//    run(cfg, cfg->entry(), init, false, 1, 2, 20, stats_enabled);
//  }
//  {
//    z_ric_domain_t init;
//    run(cfg, cfg->entry(), init, false, 1, 2, 20, stats_enabled);
//  }
//  {
//    z_term_domain_t init;
//    run(cfg, cfg->entry(), init, false, 1, 2, 20, stats_enabled);
//  }
//  {
//    z_dis_interval_domain_t init;
//    run(cfg, cfg->entry(), init, false, 1, 2, 20, stats_enabled);
//  }

  // free the CFG
  delete cfg;

  return 0;
}
