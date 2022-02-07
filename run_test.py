#!/usr/bin/python3

import os
import subprocess
import time
import signal

runner = 'time make -j -C bp_top/syn build_dump.sc sim_dump.sc COSIM_P=1 CMT_TRACE_P=1 PC_PROFILE_P=1 BRANCH_PROFILE_P=1 CORE_PROFILE_P=1 NPC_TRACE_P=1 DRAM_TRACE_P=1 '
cleaner = 'make -C bp_top/syn clean.sc'
#testbin = '/mada/users/rkjayara/projs/mpdt/ariane/testbin/'
#testbin = '/home/ramper/projs/mpdt/tmp/testbin/'
testbin = '/mada/users/rkjayara/projs/mpdt/tmp/testbin/'
#runs = '/home/ramper/projs/mpdt/tmp/runs/'
runs = '/mada/users/rkjayara/projs/mpdt/tmp/runs/'
testsuite = 'SUITE=riscv-tests '
rn0 = 'RN=0 '
rn1 = 'RN=1 '
prog = 'PROG='
#cppath = '/home/ramper/projs/mpdt/black-parrot-sim/rtl/bp_top/syn/results/verilator/bp_tethered.e_bp_default_cfg.none.sim.riscv-tests.'
cppath = '/mada/users/rkjayara/projs/mpdt/black-parrot-sim/rtl/bp_top/syn/results/verilator/bp_tethered.e_bp_default_cfg.none.sim.riscv-tests.'
counter = 0

print("CLEANING")
subprocess.call(cleaner, shell=True)
print("INITIAL MAKE")
init_runner = runner + rn0
subprocess.call(init_runner, shell=True)

for isa_test in os.listdir(testbin):
    print("\nRunning Test: " + isa_test)
    print("Making dirs")
    path0 = runs + '0/'
    path1 = runs + '1/'
    isa_path = runs + isa_test + '/'
    mk0 = 'mkdir ' + path0
    mk1 = 'mkdir ' + path1
    mkisa = 'mkdir ' + isa_path
    subprocess.call(mk0, shell=True)
    subprocess.call(mk1, shell=True)
    subprocess.call(mkisa, shell=True)
    #print("CLEANING 0")
    #subprocess.call(cleaner, shell=True)
    cmd0 = runner + rn0 + testsuite + prog + isa_test
    cmd1 = runner + rn1 + testsuite + prog + isa_test
    cp_cmd0 = 'cp ' + cppath + isa_test + '/* ' + path0
    cp_cmd1 = 'cp ' + cppath + isa_test + '/* ' + path1
    cp_cmdisa0 = 'mv ' + path0 + ' ' + isa_path
    cp_cmdisa1 = 'mv ' + path1 + ' ' + isa_path
    print("RUNNING 0")
    out_file0 = path0 + 'output.txt'
    f_out0 = open(out_file0, 'w')
    p = subprocess.Popen(cmd0, stdout = f_out0, stderr = f_out0, shell=True, universal_newlines=True, preexec_fn=os.setsid)
    time_started = time.time()
    try:
        stdout, stderr = p.communicate(timeout=180)
    except subprocess.TimeoutExpired:
        os.killpg(os.getpgid(p.pid), signal.SIGQUIT)
        stdout, stderr = p.communicate()
        time_delta = time.time() - time_started
        f_out0.write("TIME DELTA: " + str(time_delta))
    f_out0.close()
    subprocess.call(cp_cmd0, shell=True)
    #print("CLEANING 1")
    #subprocess.call(cleaner, shell=True)
    print("RUNNING 1")
    out_file1 = path1 + 'output.txt'
    f_out1 = open(out_file1, 'w')
    p = subprocess.Popen(cmd1, stdout = f_out1, stderr = f_out1, shell=True, universal_newlines=True, preexec_fn=os.setsid)
    time_started = time.time()
    try:
        stdout, stderr = p.communicate(timeout=180)
    except subprocess.TimeoutExpired:
        os.killpg(os.getpgid(p.pid), signal.SIGQUIT)
        stdout, stderr = p.communicate()
        time_delta = time.time() - time_started
        f_out1.write("TIME DELTA: " + str(time_delta))
    f_out1.close()
    subprocess.call(cp_cmd1, shell=True)
    print("Copying into isa_dir")
    subprocess.call(cp_cmdisa0, shell=True)
    subprocess.call(cp_cmdisa1, shell=True)
    print("Completed running : " + isa_test)
    counter = counter + 1
    print("counter: " + str(counter))
#    if counter == 1:
#        break
