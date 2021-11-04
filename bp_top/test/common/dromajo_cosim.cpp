#include "svdpi.h"
#include <iostream>
#include "dromajo_cosim.h"
#include "stdlib.h"
#include <string>
#include <vector>
#include <cstdio>

using namespace std;

dromajo_cosim_state_t* dromajo_pointer;
vector<bool>* finish;
char init = 0;
int run_num = -1;
FILE* mpdt_reader;
int read_ite = 0;

uint64_t counter = 0;

typedef struct reader_t{
  uint32_t                                cycle;
  uint32_t                                hartid;
  uint64_t                                pc;
  uint32_t                                opcode;
  uint64_t                                inst_cnt;
  uint32_t                                rd_addr;
  uint64_t                                data;
} reader_t;

reader_t reader[10000] = {{0}};

void struct_reader() {
  if(run_num == 1) {
    cout << "READING FILE START !!!!!!!!!!!!!!" <<  endl;
    mpdt_reader = fopen("/mada/users/rkjayara/projs/mpdt/tmp/runs/0/commit_0.trace", "r");
  
    if(mpdt_reader != NULL) {
      cout << "READING FROM run0 commit_0.trace FILE" << endl;
      while(fscanf(mpdt_reader, "%032d %08x %016x %08x %016x %08x %016x\n", &reader[read_ite].cycle, &reader[read_ite].hartid, &reader[read_ite].pc, &reader[read_ite].opcode, &reader[read_ite].inst_cnt, &reader[read_ite].rd_addr, &reader[read_ite].data) != EOF) {
        printf("cycle: %032d hartid: %08x pc: %016x opcode: %08x inst_cnt: %016x rd_addr: %08x data: %016x\n", reader[read_ite].cycle, reader[read_ite].hartid, reader[read_ite].pc, reader[read_ite].opcode, reader[read_ite].inst_cnt, reader[read_ite].rd_addr, reader[read_ite].data);
        ++read_ite;
      }
      cout << "READ " << read_ite << " LINES IN TOTAL" << endl;
    }
    else {
      cout << "FILE READ ERROR" << endl;
      cout << "FILE READ ERROR" << endl;
      cout << "FILE READ ERROR" << endl;
      cout << "FILE READ ERROR" << endl;
      cout << "FILE READ ERROR" << endl;
      exit(1);
    }
  }
}


//extern "C" void dromajo_init(char* cfg_f_name, int hartid, int ncpus, int memory_size, bool checkpoint, bool amo_en, svBit run_num_get) 

extern "C" void dromajo_init(char* cfg_f_name, int hartid, int ncpus, int memory_size, bool checkpoint, bool amo_en) {
  if (!hartid) {
    if (!init) {
      init = 1;
      cout << "Running with Dromajo cosimulation" << endl;
      
      if(run_num == 1)
       struct_reader();

      finish = new vector<bool>(ncpus, false);

      char dromajo_str[50];
      sprintf(dromajo_str, "dromajo");
      char ncpus_str[50];
      sprintf(ncpus_str, "--ncpus=%d", ncpus);
      char memsize_str[50];
      sprintf(memsize_str, "--memory_size=%d", memory_size);
      char mmio_str[50];
      sprintf(mmio_str, "--mmio_range=0x20000:0x80000000");
      char load_str[50];
      sprintf(load_str, "--load=prog");
      char amo_str[50];
      sprintf(amo_str, "--enable_amo");
      char prog_str[50];
      sprintf(prog_str, "prog.elf");

      if (checkpoint) {
        if (amo_en) {
          char* argv[] = {dromajo_str, ncpus_str, memsize_str, mmio_str, amo_str, load_str, prog_str};
          dromajo_pointer = dromajo_cosim_init(7, argv);
        }
        else {
          char* argv[] = {dromajo_str, ncpus_str, memsize_str, mmio_str, load_str, prog_str};
          dromajo_pointer = dromajo_cosim_init(6, argv);
        }
      }
      else {
        if (amo_en) {
          char* argv[] = {dromajo_str, ncpus_str, memsize_str, mmio_str, amo_str, prog_str};
          dromajo_pointer = dromajo_cosim_init(6, argv);
        }
        else {
          char* argv[] = {dromajo_str, ncpus_str, memsize_str, mmio_str, prog_str};
          dromajo_pointer = dromajo_cosim_init(5, argv);
        }
      }
    }
  }
}

extern "C" bool dromajo_step(int      hartid,
                             uint64_t pc,
                             uint32_t insn,
                             uint64_t wdata,
                             uint64_t mstatus) {
  int exit_code = dromajo_cosim_step(dromajo_pointer, 
                                     hartid,
                                     pc,
                                     insn,
                                     wdata,
                                     mstatus,
                                     true,
                                     false);
  if (exit_code != 0)
    return true;
  else
    return false;
}

extern "C" void dromajo_trap(int hartid, uint64_t cause) {
  dromajo_cosim_raise_trap(dromajo_pointer, hartid, cause, false);
}

extern "C" bool get_finish(int hartid) {
  if (!finish)
    return false;
  return finish->at(hartid);
}

extern "C" void set_finish(int hartid) {
  finish->at(hartid) = true;
}

extern "C" bool check_terminate() {
  if (!finish)
    return false;

  for (int i = 0; i < finish->size(); i++)
    if (finish->at(i) == false)
      return false;
  return true;
}

extern "C" void dromajo_printer() {
  if(counter % 1000 == 0)
    cout << "RUN: " << run_num << "Counter at: " << counter << endl;
  counter++;
}

extern "C" void set_run_num(svBit run_num_get) {
  if(run_num_get == (svBit)1) {
    run_num = 1;
    cout << "IN DROMAJO SET RUN NUMBER TO: " << run_num << endl;
    cout << "IN DROMAJO SET RUN NUMBER TO: " << run_num << endl;
    cout << "IN DROMAJO SET RUN NUMBER TO: " << run_num << endl;
  }
  else {
    run_num = 0;
    cout << "IN DROMAJO SET RUN NUMBER TO: " << run_num << endl;
    cout << "IN DROMAJO SET RUN NUMBER TO: " << run_num << endl;
    cout << "IN DROMAJO SET RUN NUMBER TO: " << run_num << endl;
  }
}
