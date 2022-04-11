#include "svdpi.h"
#include <iostream>
#include "dromajo_cosim.h"
#include <string>
#include <vector>
#include <cstdio>
#include "verilated_vcd_c.h"
//#include <cstdlib>>

using namespace std;

dromajo_cosim_state_t* dromajo_pointer;
vector<bool>* finish;
char init = 0;
int run_num = 0;
FILE* mpdt_c_reader;
FILE* mpdt_s_reader;
FILE* mpdt_p_reader;
FILE* mpdt_i_reader;
FILE* pc_d;
FILE* iC_d;
int c_read_ite = 0, s_read_ite = 0, p_read_ite = 0, i_read_ite = 0;
int cur_idx_c = 0, cur_idx_s = 0, cur_idx_p = 0, cur_idx_i = 0;
uint64_t prev_pc_s = 0;

uint64_t counter = 0, match_counter = 0;

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
  uint32_t                                dat;
  uint32_t                                val;
  uint32_t                                 rn;
} fepc_reader_t;

typedef struct iCache_reader_t {
  uint32_t                                cycle;
  uint64_t                                vaddr;
  uint32_t                                data;
  uint32_t                                 val;
  uint32_t                                rn;
} iCache_reader_t;

typedef struct mpdt_holder_t {
  uint32_t                                start_cycle;
  uint32_t                                end_cycle;
  uint64_t                                start_addr;
  uint64_t                                end_addr;
  uint32_t                                fake_inst;
}mpdt_holder_t;

typedef enum 
{
  d_e_lce_mode_uncached = 0
  ,d_e_lce_mode_normal  = 1
  ,d_e_lce_mode_nonspec = 2
} d_bp_lce_mode_e;

d_bp_lce_mode_e spec_mode_select = d_e_lce_mode_nonspec;

commit_reader_t c_reader[5000000] = {{0}};
stall_reader_t  s_reader[5000000] = {{0}};
fepc_reader_t   p_reader[5000000] = {{0}};
iCache_reader_t i_reader[5000000] = {{0}};

mpdt_holder_t   mpdt_now = {0};

int nbf_complete = 0;

uint32_t d_cycle_cnt = 0;

int mpdt_current_flag = 0,  prev_mpdt_cyc = 0;

uint32_t inst_arr[16] = {0};
int inst_idx_max = 0, inst_idx_cur = 0;
FILE* inst_reader;

bool n_not_f = false;

void inst_f_reader() {
  if(run_num == 1) {
    cout << "READING INST FILE" << endl;
    inst_reader = fopen("/mada/users/rkjayara/projs/mpdt/tmp/faker/csr.txt", "r");
    //inst_reader = fopen("/home/ramper/projs/mpdt/tmp/faker/fen.txt", "r");
    if(inst_reader != NULL) {
      while(fscanf(inst_reader, "%x\n", &inst_arr[inst_idx_max]) != EOF) {
        printf("read inst: %08x at idx %d", inst_arr[inst_idx_max], inst_idx_max);
        ++inst_idx_max;
      }
      cout << "READ " << inst_idx_max << " LINES TOTAL FROM INST" << endl;
    }
    else {
      cout << "INST FILE READ ERROR" << endl;
      cout << "INST FILE READ ERROR" << endl;
      cout << "INST FILE READ ERROR" << endl;
      cout << "INST FILE READ ERROR" << endl;
      cout << "INST FILE READ ERROR" << endl;
      exit(1);
    }
  }
}

void struct_reader() {
  if(run_num == 1) {
    cout << "READING FILE START !!!!!!!!!!!!!!" <<  endl;

    mpdt_c_reader = fopen("/soe/rkjayara/projs/mpdt/spec_d/new/0/commit_0.trace", "r");
    //mpdt_c_reader = fopen("/home/ramper/projs/mpdt/tmp/runs/0/commit_0.trace", "r");
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

    mpdt_s_reader = fopen("/soe/rkjayara/projs/mpdt/spec_d/new/0/stall_0.trace", "r");
    //mpdt_s_reader = fopen("/home/ramper/projs/mpdt/tmp/runs/0/stall_0.trace", "r");
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

    mpdt_p_reader = fopen("/soe/rkjayara/projs/mpdt/spec_d/new/0/pc_dump.txt", "r");
    //mpdt_p_reader = fopen("/home/ramper/projs/mpdt/tmp/runs/0/pc_dump.txt", "r");
    if(mpdt_p_reader != NULL) {
      cout << "READING FROM run0 pc_dump.txt FILE" << endl;
      while(fscanf(mpdt_p_reader, "%d %x %x %08x %x %1x\n", &p_reader[p_read_ite]. cycle, &p_reader[p_read_ite].npc, &p_reader[p_read_ite].fpc, &p_reader[p_read_ite].dat, &p_reader[p_read_ite].val, &p_reader[p_read_ite].rn) != EOF) {
        //printf("cycle: %d npc: %x fpc: %x dat: %08x val: %x rn: %1x\n", p_reader[p_read_ite].cycle, p_reader[p_read_ite].npc, p_reader[p_read_ite].fpc, p_reader[p_read_ite].dat, p_reader[p_read_ite].val, p_reader[p_read_ite].rn);
        ++p_read_ite;
      }
      cout << "READ " << p_read_ite << " LINES IN TOTAL FOR PCDUMP" << endl;
    }
    else {
      cout << "PCDUMP FILE READ ERROR" << endl;
      cout << "PCDUMP FILE READ ERROR" << endl;
      cout << "PCDUMP FILE READ ERROR" << endl;
      cout << "PCDUMP FILE READ ERROR" << endl;
      cout << "PCDUMP FILE READ ERROR" << endl;
      exit(1);
    }

    mpdt_i_reader = fopen("/soe/rkjayara/projs/mpdt/spec_d/new/0/iC_dump.txt", "r");
    //mpdt_i_reader = fopen("/home/ramper/projs/mpdt/tmp/runs/0/iC_dump.txt", "r");
    if(mpdt_i_reader != NULL) {
      cout << "READING FROM run0 iC_dump.txt FILE" << endl;
      while(fscanf(mpdt_i_reader, "%d %x %08x %x %1x\n", &i_reader[i_read_ite].cycle, &i_reader[i_read_ite].vaddr, &i_reader[i_read_ite].data, &i_reader[i_read_ite].val, &i_reader[i_read_ite].rn) != EOF) {
        //printf("cycle: %d vaddr: %x data %08x val: %x rn: %1x\n", i_reader[i_read_ite].cycle, i_reader[i_read_ite].vaddr, i_reader[i_read_ite].data, i_reader[i_read_ite].val, i_reader[i_read_ite].rn);
        ++i_read_ite;
      }
      cout << "READ " << i_read_ite << " LINES IN TOTAL FOR ICDUMP" << endl;
    }
    else {
      cout << "ICDUMP FILE READ ERROR" << endl;
      cout << "ICDUMP FILE READ ERROR" << endl;
      cout << "ICDUMP FILE READ ERROR" << endl;
      cout << "ICDUMP FILE READ ERROR" << endl;
      cout << "ICDUMP FILE READ ERROR" << endl;
      exit(1);
    }
  }
}


//extern "C" void dromajo_init(char* cfg_f_name, int hartid, int ncpus, int memory_size, bool checkpoint, bool amo_en, svBit run_num_get) 

extern "C" void dromajo_init(char* cfg_f_name, int hartid, int ncpus, int memory_size, bool checkpoint, bool amo_en) {
  if (!hartid) {
    if (!init) {
      init = 1;
      srand (time(NULL));
      cout << "Running with Dromajo cosimulation" << endl;
      pc_d = fopen("pc_dump.txt", "w");
      iC_d = fopen("iC_dump.txt", "w");
      if(run_num == 1) {
        struct_reader();
        inst_f_reader();
      }

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

  if(counter > 250000)
    exit(0);
}

extern "C" void set_run_num(svBit run_num_get) {
  if(run_num_get == (svBit)1) {
    run_num = 1;
    cout << "IN DROMAJO SET RUN NUMBER TO: " << run_num << " AND SPEC_MODE: " << spec_mode_select << endl;
    cout << "IN DROMAJO SET RUN NUMBER TO: " << run_num << " AND SPEC_MODE: " << spec_mode_select << endl;
    cout << "IN DROMAJO SET RUN NUMBER TO: " << run_num << " AND SPEC_MODE: " << spec_mode_select << endl;
  }
  else {
    run_num = 0;
    cout << "IN DROMAJO SET RUN NUMBER TO: " << run_num << " AND SPEC_MODE: " << spec_mode_select << endl;
    cout << "IN DROMAJO SET RUN NUMBER TO: " << run_num << " AND SPEC_MODE: " << spec_mode_select << endl;
    cout << "IN DROMAJO SET RUN NUMBER TO: " << run_num << " AND SPEC_MODE: " << spec_mode_select << endl;
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

extern "C" void pc_dumper(const svBitVecVal* npc, const svBitVecVal* fpc, const svBitVecVal* dat, const svBit val) {
  if(init == 1)
    fprintf(pc_d, "%d %x %x %08x %x %1x\n", d_cycle_cnt, (uint64_t)*npc, (uint64_t)*fpc, *dat, val, run_num);
}

extern "C" void iCache_dump(const svBitVecVal* vaddr, const svBitVecVal* data_o, const svBit val) {
  if(init == 1)
    fprintf(iC_d, "%d %x %08x %x %1x\n", d_cycle_cnt, (uint64_t)*vaddr, (uint64_t)*data_o, val, run_num);
}

extern "C" void nbf_done(const svBit val)
{
  printf("GOT NBF COMPLETE FROM DUT AT CYCLE %d\n", d_cycle_cnt);
  nbf_complete = 1;
}

void next_mispredict()
{
  //cout << std::hex << "CALLED NEXT PC IDX " << cur_idx_s << " PrevPC: " << prev_pc_s << endl;
  uint64_t found_pc = 0;
  if (init == 1 && run_num == 1) {
    prev_pc_s = s_reader [cur_idx_s].pc;
    for(; cur_idx_s < s_read_ite ; ++cur_idx_s) {
      if(s_reader[cur_idx_s].operation == 13 && (uint64_t)s_reader[cur_idx_s].pc > 0x80000250) 
        if(s_reader[cur_idx_s].pc != prev_pc_s) {
          //cout << std::hex << "FOUND NEXT MISPREDUCT FROM " << prev_pc_s << " WITH " << s_reader[cur_idx_s].pc << endl;
          found_pc = s_reader[cur_idx_s].pc;
          break;
        }
    }
    while(s_reader[cur_idx_s - 1].pc == found_pc) {
      --cur_idx_s;
      if(s_reader[cur_idx_s - 1].pc != found_pc && s_reader[cur_idx_s - 1].operation == 0) {
        break;
      }
    }
  }
}

void set_mpdt_holder_cycles(uint32_t target_cycle)
{
  //wait till nbf complete
  //wait till nbf_done is called from verilog

  if(nbf_complete == 1) {

  if(n_not_f == false) {
    //printf("SET_MPDT: Got target cycle: %d\n", target_cycle);
    int cur_idx = (int)target_cycle;
    bool set_start = false;
    printf("SET_MPDT: Got target cycle: %d and idx: %d\n", target_cycle, cur_idx);
    if(target_cycle == p_reader[cur_idx].cycle) {
      if(p_reader[cur_idx].fpc == mpdt_now.start_addr) {
        mpdt_now.start_cycle = p_reader[cur_idx].cycle;
        set_start = true;
      }
      while(cur_idx < p_read_ite) {
        ++cur_idx;
        if(p_reader[cur_idx].fpc == mpdt_now.end_addr && p_reader[cur_idx].val != 0)
          break;
      }
      //printf("AFTER LOOP IDX: %d\n", cur_idx);
      if(p_reader[cur_idx].fpc == mpdt_now.end_addr){
        mpdt_now.end_cycle = p_reader[cur_idx].cycle;
      }
      else {
        printf("ERROR FINDING END CYCLE\n");
        exit(1);
      }
      if(set_start == false) {
        while(p_reader[cur_idx].cycle > prev_mpdt_cyc) {
          --cur_idx;
          if(p_reader[cur_idx].fpc == mpdt_now.start_addr && p_reader[cur_idx].val == 1) {
            mpdt_now.start_cycle = p_reader[cur_idx].cycle;
            set_start = true;
          }
        }
      } 
      if (set_start == false) {
        printf("ERROR SETTING START CYCLE\n");
        exit(1);
      }
    }
    else {
      printf("ERROR SETTING HOLDER CYCLES: COULDNT MATCH\n");
      exit(1);
    }
  }
  else if(n_not_f == true) {
    //printf("SET_MPDT: Got target cycle: %d\n", target_cycle);
    int cur_idx = (int)target_cycle;
    bool set_start = false;
    if(target_cycle == p_reader[cur_idx].cycle) {
      if(p_reader[cur_idx].npc == mpdt_now.start_addr) {
        mpdt_now.start_cycle = p_reader[cur_idx].cycle;
        set_start = true;
      }
      while(cur_idx < p_read_ite) {
        ++cur_idx;
        if(p_reader[cur_idx].npc == mpdt_now.end_addr && p_reader[cur_idx].val != 0)
          break;
      }
      //printf("AFTER LOOP IDX: %d\n", cur_idx);
      if(p_reader[cur_idx].npc == mpdt_now.end_addr){
        mpdt_now.end_cycle = p_reader[cur_idx].cycle;
      }
      else {
        printf("ERROR FINDING END CYCLE\n");
        exit(1);
      }
      if(set_start == false) {
        while(p_reader[cur_idx].cycle > prev_mpdt_cyc) {
          --cur_idx;
          if(p_reader[cur_idx].npc == mpdt_now.start_addr && p_reader[cur_idx].val == 1) {
            mpdt_now.start_cycle = p_reader[cur_idx].cycle;
            set_start = true;
          }
        }
      } 
      if (set_start == false) {
        printf("ERROR SETTING START CYCLE\n");
        exit(1);
      }
    }
    else {
      printf("ERROR SETTING HOLDER CYCLES: COULDNT MATCH\n");
      exit(1);
    }
  }

  } //done for NBF bypass
}

//*********************************************************************]
//change to not insert before NBF loader done. 
//*********************************************************************]

//void is_mpdt_helper(const svBitVecVal* npc, const svBitVecVal* fpc) {
void is_mpdt_helper() {
  mpdt_current_flag = 0;
  if(d_cycle_cnt > mpdt_now.start_cycle && d_cycle_cnt < mpdt_now.end_cycle) {
    mpdt_current_flag = 1;
    printf("SET MPDT=1 AT CYCLE %d\n", d_cycle_cnt);
  }
}

uint32_t inst_getter() {
  inst_idx_cur = rand() % inst_idx_max;
  //printf("SET RAND INDEX TO: %d\n", inst_idx_cur);
  return inst_arr[inst_idx_cur];
}

extern "C" void is_mpdt(const svBitVecVal* npc, const svBitVecVal* fpc, svBit* mpdt_flag, svBitVecVal* fake_inst, svBitVecVal* fake_addr, svBit* selector) {
  if(init == 1 && run_num == 1 && nbf_complete ==1) {    
    if(d_cycle_cnt >= s_reader[cur_idx_s].cycle && cur_idx_s != s_read_ite && d_cycle_cnt >= mpdt_now.end_cycle) {
      //printf("CUR IDX: %d IDXPC: %x CALLPC: %x CYCLE: %d\n", cur_idx_s, s_reader[cur_idx_s].pc, *npc, d_cycle_cnt);  
      prev_mpdt_cyc = mpdt_now.end_cycle;    
      next_mispredict();
      //printf("AFTER CALL: CUR IDX: %d IDXPC: %x CALLPC: %x CYCLE: %d\n", cur_idx_s, s_reader[cur_idx_s].pc, *npc, d_cycle_cnt);
      mpdt_now.start_addr = s_reader[cur_idx_s-1].pc;
      mpdt_now.end_addr   = s_reader[cur_idx_s].pc;
      
      //This can be done here or every time we detect mpdt below
      mpdt_now.fake_inst  = inst_getter();
      
      
      set_mpdt_holder_cycles(s_reader[cur_idx_s-1].cycle - 6);
      printf("\nSET START ADDR: %x START CYCLE: %d END ADDR: %x END CYCLE %d FAKE INST: %x\n", mpdt_now.start_addr, mpdt_now.start_cycle, mpdt_now.end_addr, mpdt_now.end_cycle, mpdt_now.fake_inst);
    }
    is_mpdt_helper();
    if(n_not_f == false) {
      if(mpdt_current_flag) {
        //mpdt_now.fake_inst  = inst_getter();
        *mpdt_flag = (svBit)1;
        *fake_inst = (svBitVecVal)mpdt_now.fake_inst;
        *fake_addr = (svBitVecVal)0x000000000;
        *selector  = (svBit)0;
      }
      else {
        *mpdt_flag = (svBit)0;
        *fake_inst = (svBitVecVal)0x00000000;
        *fake_addr = (svBitVecVal)0x000000000;
        *selector  = (svBit)0;
      }
    }
    else if(n_not_f == true) {
      if(mpdt_current_flag) {
        //mpdt_now.fake_inst  = inst_getter();
        *mpdt_flag = (svBit)1;
        *fake_inst = (svBitVecVal)0x00000000;
        *fake_addr = (svBitVecVal)0x080000000;
        *selector  = (svBit)1;
      }
      else {
        *mpdt_flag = (svBit)0;
        *fake_inst = (svBitVecVal)0x00000000;
        *fake_addr = (svBitVecVal)0x000000000;
        *selector  = (svBit)1;
      }
    }
  }
  else if(init == 1 && run_num == 0) {
    *mpdt_flag = (svBit)0;
    *fake_inst = (svBitVecVal)0x00000000;
    *fake_addr = (svBitVecVal)0x000000000;
    *selector  = (svBit)0;
  }
}
