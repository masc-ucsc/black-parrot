#include "svdpi.h"
#include <iostream>
#include "dromajo_cosim.h"
#include <string>
#include <vector>
#include <cstdio>
#include "verilated_vcd_c.h"
#include <cstdlib>>

using namespace std;

dromajo_cosim_state_t* dromajo_pointer;
vector<bool>* finish;
char init = 0;
int run_num = -1;
FILE* mpdt_c_reader;
FILE* mpdt_s_reader;
FILE* mpdt_f_reader;
FILE* pc_d;
int c_read_ite = 0, s_read_ite = 0, f_read_ite = 0;

uint64_t counter = 0;

typedef struct commit_reader_t {
  uint32_t                                cycle;
  uint32_t                                hartid;
  uint64_t                                pc;
  uint32_t                                opcode;
  uint64_t                                inst_cnt;
  uint32_t                                rd_addr;
  uint64_t                                data;
}commit_reader_t;

typedef struct stall_reader_t {
  uint32_t                                cycle;
  uint16_t                                x;
  uint16_t                                y;
  uint64_t                                pc;
  uint16_t                                operation;
}stall_reader_t;

typedef struct fepc_reader_t {
  uint32_t                                cycle;
  uint64_t                                npc;
  uint64_t                                fpc;
} fepc_reader_t;

commit_reader_t c_reader[10000] = {{0}};
stall_reader_t  s_reader[10000] = {{0}};
fepc_reader_t   f_reader[10000] = {{0}};

uint32_t d_cycle_cnt = 0;

void struct_reader() {
  if(run_num == 1) {
    cout << "READING FILE START !!!!!!!!!!!!!!" <<  endl;

    //mpdt_c_reader = fopen("/mada/users/rkjayara/projs/mpdt/tmp/runs/0/commit_0.trace", "r");
    mpdt_c_reader = fopen("/home/ramper/projs/mpdt/tmp/runs/0/commit_0.trace", "r");
    if(mpdt_c_reader != NULL) {
      cout << "READING FROM run0 commit_0.trace FILE" << endl;
      while(fscanf(mpdt_c_reader, "%010d %08x %016x %08x %016x %08x %016x\n", &c_reader[c_read_ite].cycle, &c_reader[c_read_ite].hartid, &c_reader[c_read_ite].pc, &c_reader[c_read_ite].opcode, &c_reader[c_read_ite].inst_cnt, &c_reader[c_read_ite].rd_addr, &c_reader[c_read_ite].data) != EOF) {
        //printf("cycle: %010d hartid: %08x pc: %016x opcode: %08x inst_cnt: %016x rd_addr: %08x data: %016x\n", c_reader[c_read_ite].cycle, c_reader[c_read_ite].hartid, c_reader[c_read_ite].pc, c_reader[c_read_ite].opcode, c_reader[c_read_ite].inst_cnt, c_reader[c_read_ite].rd_addr, c_reader[c_read_ite].data);
        ++c_read_ite;
      }
      cout << "READ " << c_read_ite << " LINES IN TOTAL FOR COMMIT" << endl;
    }
    else {
      cout << "COMMIT FILE READ ERROR" << endl;
      cout << "COMMIT FILE READ ERROR" << endl;
      cout << "COMMIT FILE READ ERROR" << endl;
      cout << "COMMIT FILE READ ERROR" << endl;
      cout << "COMMIT FILE READ ERROR" << endl;
      exit(1);
    }

    //mpdt_s_reader = fopen("/mada/users/rkjayara/projs/mpdt/tmp/runs/0/stall_0.trace", "r");
    mpdt_s_reader = fopen("/home/ramper/projs/mpdt/tmp/runs/0/stall_0.trace", "r");
    if(mpdt_s_reader !=NULL) {
      cout << "READING FROM run0 stall_0.trace FILE" << endl;
      while(fscanf(mpdt_s_reader, "%010d,%04x,%04x,%016x,%04d\n", &s_reader[s_read_ite].cycle, &s_reader[s_read_ite].x, &s_reader[s_read_ite].y, &s_reader[s_read_ite].pc, &s_reader[s_read_ite].operation) != EOF) {
        //printf("cycle: %010d x: %04x y: %04x pc: %016x operation: %04d\n", s_reader[s_read_ite].cycle, s_reader[s_read_ite].x, s_reader[s_read_ite].y, s_reader[s_read_ite].pc, s_reader[s_read_ite].operation);
        ++s_read_ite;
      }
      cout << "READ " << s_read_ite << " LINES IN TOTAL FOR STALL" << endl;
    }
    else {
      cout << "STALL FILE READ ERROR" << endl;
      cout << "STALL FILE READ ERROR" << endl;
      cout << "STALL FILE READ ERROR" << endl;
      cout << "STALL FILE READ ERROR" << endl;
      cout << "STALL FILE READ ERROR" << endl;
      exit(1);
    }

    //mpdt_s_reader = fopen("/mada/users/rkjayara/projs/mpdt/tmp/runs/0/pc_dump.txt", "r");
    mpdt_f_reader = fopen("/home/ramper/projs/mpdt/tmp/runs/0/pc_dump.txt", "r");
    if(mpdt_f_reader != NULL) {
      cout << "READING FROM run0 pc_dump.txt FILE" << endl;
      while(fscanf(mpdt_f_reader, "%d %x %x\n", &f_reader[f_read_ite].cycle, &f_reader[f_read_ite].npc, &f_reader[f_read_ite].fpc) != EOF) {
        //printf("cycle: %d npc: %x %x\n", f_reader[f_read_ite].cycle, f_reader[f_read_ite].npc, f_reader[f_read_ite].fpc);
        ++f_read_ite;
      }
      cout << "READ " << f_read_ite << " LINES IN TOTAL FOR PCDUMP" << endl;
    }
    else {
      cout << "PCDUMP FILE READ ERROR" << endl;
      cout << "PCDUMP FILE READ ERROR" << endl;
      cout << "PCDUMP FILE READ ERROR" << endl;
      cout << "PCDUMP FILE READ ERROR" << endl;
      cout << "PCDUMP FILE READ ERROR" << endl;
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
      pc_d = fopen("pc_dump.txt", "w");
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
    cout << "RUN: " << run_num << " Counter at: " << counter << " cycle at: " << d_cycle_cnt << endl;
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

extern "C" void get_cycle(const svBitVecVal* cycle_cnt) {
  //int bcyc    = (int) bit_cycle_cnt;
  //double cyc  = (double) cycle_cnt;
  //if (bcyc %  == 0) 
    //cout << "BIT CYCLE: " << *bcyc << " CYCLE: " << *cyc << endl;
  d_cycle_cnt = (uint32_t)*cycle_cnt;
  //printf("BIT CYCLE: %ld LOGIC CYCLE: %ld \n", d_cycle_cnt, *cycle_cnt);
}

extern "C" void put_cycle(svBitVecVal* commit_cycle_cnt) {
  *commit_cycle_cnt = (svBitVecVal) d_cycle_cnt;
}

extern "C" void pc_dumper(const svBitVecVal* npc, const svBitVecVal* fpc) {
  if(init == 1)
    fprintf(pc_d, "%d %x %x\n", d_cycle_cnt, (uint64_t)*npc, (uint64_t)*fpc);
}
