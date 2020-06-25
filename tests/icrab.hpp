#pragma once
#include "./crab_lang.hpp"

// For top-down inter-procedural analysis parameters
#include <crab/analysis/inter/top_down_inter_analyzer.hpp>



// To run abstract domains defined over integers
template<typename BUDom, typename TDDom>
extern void z_bu_inter_run_and_check(crab::cg_impl::z_cg_t* cg,
				     BUDom bu_top, TDDom td_top,
				     bool run_liveness,
				     unsigned widening, 
				     unsigned narrowing, 
				     unsigned jump_set_size,
				     bool enable_stats);

// To run abstract domains defined over integers
typedef crab::analyzer::top_down_inter_analyzer_parameters
<crab::cg_impl::z_cg_ref_t> td_inter_params_t;

template<typename Dom>
extern void z_td_inter_run_and_check(crab::cg_impl::z_cg_t* cg,
				     Dom init,
				     td_inter_params_t params,
				     bool print_checks,
				     bool print_invariants,
				     bool enable_stats);







// To run abstract domains defined over integers
template<typename BUDom, typename TDDom>
void bu_inter_run(crab::cg_impl::z_cg_t* cg,
		  BUDom bu_top, TDDom td_top,
		  bool run_liveness,
		  unsigned widening, 
		  unsigned narrowing, 
		  unsigned jump_set_size,
		  bool enable_stats) {
  #ifdef USE_GENERIC_WRAPPER
  using namespace crab::domain_impl;  
  z_abs_domain_t bu_top_wrapper(bu_top);
  z_abs_domain_t td_top_wrapper(td_top);  
  z_bu_inter_run_and_check(cg, bu_top_wrapper, td_top_wrapper,
			   run_liveness, widening, narrowing, jump_set_size,
			   enable_stats);

  #else
  z_bu_inter_run_and_check(cg, bu_top, td_top,
			   run_liveness, widening, narrowing, jump_set_size,
			   enable_stats);
  #endif 
}

// To run abstract domains defined over integers
typedef crab::analyzer::top_down_inter_analyzer_parameters
<crab::cg_impl::z_cg_ref_t> td_inter_params_t;

template<typename Dom>
void td_inter_run(crab::cg_impl::z_cg_t* cg,
		  Dom init,
		  td_inter_params_t params,
		  bool print_checks,
		  bool print_invariants,
		  bool enable_stats) {
  #ifdef USE_GENERIC_WRAPPER
  using namespace crab::domain_impl;
  z_abs_domain_t init_wrapper(init);
  z_td_inter_run_and_check(cg, init_wrapper, params, print_checks, print_invariants, enable_stats);  
  #else
  z_td_inter_run_and_check(cg, init, params, print_checks, print_invariants, enable_stats);
  #endif 
}


