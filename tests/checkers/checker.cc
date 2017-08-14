#include "../program_options.hpp"
#include "../common.hpp"
#include <crab/checkers/base_property.hpp>
#include <crab/checkers/null.hpp>
#include <crab/checkers/div_zero.hpp>
#include <crab/checkers/assertion.hpp>
#include <crab/checkers/checker.hpp>
#include <crab/analysis/dataflow/assertion_crawler.hpp>

using namespace std;
using namespace crab::analyzer;
using namespace crab::cfg;
using namespace crab::cfg_impl;
using namespace crab::domain_impl;
using namespace crab::checker;


z_cfg_t* cfg1 (variable_factory_t &vfac) 
{

  // entry and exit block
  z_cfg_t* cfg = new z_cfg_t("b0","b3",PTR);
  // adding blocks
  z_basic_block_t& b0 = cfg->insert ("b0");
  z_basic_block_t& b1 = cfg->insert ("b1");
  z_basic_block_t& b2 = cfg->insert ("b2");
  z_basic_block_t& b3 = cfg->insert ("b3");
  // adding control flow
  b0 >> b1; b0 >> b2; b1 >> b3; b2 >> b3;
  

  // definining program variables
  varname_t p = vfac ["p"];
  varname_t q1 = vfac ["q1"];
  varname_t q2 = vfac ["q2"];
  varname_t r = vfac ["r"];
  z_var nd (vfac ["nd"]);
  // adding statements
  b0.ptr_new_object (p , 1);  // p = malloc (...);
  b0.ptr_new_object (q1, 2);  // q1 = malloc (...);
  b0.ptr_new_object (q2, 3);  // q2 = malloc (...);
  b0.havoc (nd.name ());
  b1.assume (nd >= 1);
  b2.assume (nd <= 0);
  b1.ptr_store (p, q1);  // *p = q1
  b2.ptr_store (p, q2);  // *p = q2
  b3.ptr_load (r, p);    // r = *p
  return cfg;
}


z_cfg_t* cfg2 (variable_factory_t &vfac)  {

  // Definining program variables
  z_var i (vfac ["i"]);
  z_var x (vfac ["x"]);
  z_var y (vfac ["y"]);
  varname_t p (vfac ["p"]);
  varname_t q (vfac ["q"]);
  // entry and exit block
  z_cfg_t* cfg = new z_cfg_t("entry","ret",PTR);
  // adding blocks
  z_basic_block_t& entry = cfg->insert ("entry");
  z_basic_block_t& bb1   = cfg->insert ("bb1");
  z_basic_block_t& bb1_t = cfg->insert ("bb1_t");
  z_basic_block_t& bb1_f = cfg->insert ("bb1_f");
  z_basic_block_t& bb2   = cfg->insert ("bb2");
  z_basic_block_t& ret   = cfg->insert ("ret");
  // adding control flow 
  entry >> bb1;
  bb1 >> bb1_t; bb1 >> bb1_f;
  bb1_t >> bb2; bb2 >> bb1; bb1_f >> ret;
  // adding statements
  entry.assign (i, 0);
  entry.assign (x, 1);
  entry.assign (y, 0);
  entry.ptr_null (p);
  bb1_t.assume (i <= 99);
  bb1_f.assume (i >= 100);
  bb2.add(x,x,y);
  bb2.add(y,y,1);
  bb2.ptr_new_object (q, 1);
  bb2.ptr_assign (p, q, z_number(4));
  bb2.add(i, i, 1);
  ret.assume (x <= y);
  ret.assertion (i == 100);
  ret.assertion (i >= 200);
  ret.assertion (x >= y);
  ret.assertion (x >= 200);


  return cfg;
}

void check (z_cfg_ref_t cfg, variable_factory_t& vfac) {

  // Each checker is associated to one analyzer
  typedef intra_fwd_analyzer<z_cfg_ref_t, z_num_null_domain_t> null_analyzer_t;
  typedef intra_checker<null_analyzer_t> null_checker_t;
  typedef intra_fwd_analyzer<z_cfg_ref_t, z_sdbm_domain_t> num_analyzer_t;
  typedef intra_checker<num_analyzer_t> num_checker_t;

  // We can have multiple properties per analyzer
  typedef null_property_checker<null_analyzer_t> null_prop_null_checker_t;
  typedef div_zero_property_checker<num_analyzer_t> div_zero_prop_num_checker_t;
  typedef assert_property_checker<num_analyzer_t> assert_prop_num_checker_t;


  // Run liveness (optionally) and print cfg
  liveness<z_cfg_ref_t> live (cfg);  
  crab::outs() << cfg << "\n";
  
  // Run nullity analysis
  null_analyzer_t null_a (cfg, z_num_null_domain_t::top (), &live);
  null_a.run ();
  crab::outs() << "Analysis using " << z_num_null_domain_t::getDomainName () << "\n";
  for (auto &b : cfg)  {
    auto pre = null_a.get_pre (b.label ());
    auto post = null_a.get_post (b.label ());
    crab::outs() << get_label_str (b.label ()) << "=" 
              << pre 
              << " ==> "
              << post << "\n";
  }

  // Run numerical analysis
  num_analyzer_t num_a (cfg, z_sdbm_domain_t::top (),  &live);
  num_a.run ();
  crab::outs() << "Analysis using " << z_sdbm_domain_t::getDomainName () << "\n";
  for (auto &b : cfg)  {
    auto pre = num_a.get_pre (b.label ());
    auto post = num_a.get_post (b.label ());
    crab::outs() << get_label_str (b.label ()) << "=" 
              << pre 
              << " ==> "
              << post << "\n";
  }

  
  // Run the checkers with several properties
  // A checker can take any property checker associated to same
  // analyzer.
  const int verbose = 3;
  {
    typename null_checker_t::prop_checker_ptr prop1 (new null_prop_null_checker_t (verbose));
    null_checker_t checker (null_a, {prop1});
    checker.run ();
    checker.show (crab::outs());
  }

  {
    typename num_checker_t::prop_checker_ptr prop1 (new div_zero_prop_num_checker_t (verbose));
    typename num_checker_t::prop_checker_ptr prop2 (new assert_prop_num_checker_t (verbose));
    num_checker_t checker (num_a, {prop1, prop2});

    checker.run ();
    checker.show (crab::outs());
  }

}

int main (int argc, char**argv) {

  SET_TEST_OPTIONS(argc,argv)

  variable_factory_t vfac;
  z_cfg_t* p1 = cfg1 (vfac);
  check (*p1, vfac);
  z_cfg_t* p2 = cfg2 (vfac);

  // To test the assertion crawler analysis: we do nothing with that
  // for now.
  crab::analyzer::assertion_crawler<z_cfg_ref_t> assert_crawler(*p2);
  assert_crawler.exec();
  crab::outs () << "\n" << assert_crawler << "\n";
  
  check (*p2, vfac);

  delete p1;
  delete p2;

  return 0;
}
