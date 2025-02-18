#include <stdlib.h>
#include <verilated_fst_c.h>
#include <verilated_cov.h>
#include "verilated_vcd_c.h"

#include "Vtestbench.h"
#include "Vtestbench__Dpi.h"
#include "bsg_nonsynth_dpi_clock_gen.hpp"
using namespace bsg_nonsynth_dpi;

#include <getopt.h>
#include <string>
#include <cstdlib>
#include <cstdio>
using namespace std;

int main(int argc, char **argv) {

  int run_num       = -1;
  int mpdt_verbose  = 0;

  while(1) {
    static struct option long_options[] = {
      {"run-Num",       required_argument,  0,  'N'}
    };
    int option_index;
    int c = getopt_long(argc, argv, "-chpm:N:", long_options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'N': run_num = atoi(optarg);     break;
    }
  }


  if(run_num != -1) {
    std::cout << "IN TB.CPP: GOT RUN NUM: " << run_num << std::endl;
    std::cout << "IN TB.CPP: GOT RUN NUM: " << run_num << std::endl;
    std::cout << "IN TB.CPP: GOT RUN NUM: " << run_num << std::endl;
  }


  Verilated::commandArgs(argc, argv);
  Verilated::traceEverOn(VM_TRACE_FST);
  Verilated::assertOn(false);

  Vtestbench *tb = new Vtestbench("testbench");

  svScope g_scope = svGetScopeFromName("testbench");
  svSetScope(g_scope);

  tb->run_num = run_num;
  // Let clock generators register themselves.
  tb->eval();

  // Use me to find the correct scope of your DPI functions
  //Verilated::scopesDump();
#if VM_TRACE
  Verilated::traceEverOn(true);
  VerilatedVcdC* tfp = new VerilatedVcdC;
  tb->trace(tfp, 99);
  tfp->open("vcd.vcd");
#endif


#if VM_TRACE_FST
  std::cout << "Opening dump file" << std::endl;
  VerilatedFstC* wf = new VerilatedFstC;
  tb->trace(wf, 10);
  wf->open("dump.fst");
#endif

  while(tb->reset_i == 1) {
    bsg_timekeeper::next();
    tb->eval();
    #if VM_TRACE_FST
      wf->dump(sc_time_stamp());
    #endif
    #if VM_TRACE
      tfp->dump(sc_time_stamp());
    #endif
  }

  Verilated::assertOn(true);

  while (!Verilated::gotFinish()) {
    bsg_timekeeper::next();
    tb->eval();
    #if VM_TRACE_FST
      wf->dump(sc_time_stamp());
    #endif
    #if VM_TRACE
      tfp->dump(sc_time_stamp());
    #endif
  }
  std::cout << "Finishing test" << std::endl;

#if VM_COVERAGE
  std::cout << "Writing coverage" << std::endl;
  VerilatedCov::write("coverage.dat");
#endif

  std::cout << "Executing final" << std::endl;
  tb->final();

#if VM_TRACE
  tfp->close();
#endif

  #if VM_TRACE_FST
    std::cout << "Closing dump file" << std::endl;
    wf->close();
  #endif

  std::cout << "Exiting" << std::endl;
  exit(EXIT_SUCCESS);
}

