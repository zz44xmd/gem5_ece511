# Copyright (c) 2012-2013 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2006-2008 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Simple test script
#
# "m5 test.py"

import argparse
import m5

from common import Options
from common import Simulation
from common import CacheConfig
from common import MemConfig
from common.FileSystemConfig import config_filesystem
from common.Caches import *
from common.cpu2000 import *

from m5.objects import *

import sys
import os

def get_processes(args):
    """Interprets provided args and returns a list of processes"""

    multiprocesses = []
    inputs = []
    outputs = []
    errouts = []
    pargs = []

    workloads = args.cmd.split(';')
    if args.input != "":
        inputs = args.input.split(';')
    if args.output != "":
        outputs = args.output.split(';')
    if args.errout != "":
        errouts = args.errout.split(';')
    if args.options != "":
        pargs = args.options.split(';')

    idx = 0
    for wrkld in workloads:
        process = Process(pid = 100 + idx)
        process.executable = wrkld
        process.cwd = os.getcwd()
        process.gid = os.getgid()

        if args.env:
            with open(args.env, 'r') as f:
                process.env = [line.rstrip() for line in f]

        if len(pargs) > idx:
            process.cmd = [wrkld] + pargs[idx].split()
        else:
            process.cmd = [wrkld]

        if len(inputs) > idx:
            process.input = inputs[idx]
        if len(outputs) > idx:
            process.output = outputs[idx]
        if len(errouts) > idx:
            process.errout = errouts[idx]

        multiprocesses.append(process)
        idx += 1

    if args.smt:
        assert(args.cpu_type == "DerivO3CPU")
        return multiprocesses, idx
    else:
        return multiprocesses, 1

############################################################ Argument and Sanity Check
parser = argparse.ArgumentParser()
Options.addCommonOptions(parser)
Options.addSEOptions(parser)
args = parser.parse_args()

program = args.cmd.split(';')[0]
if (program == None or program == ""):
        raise ValueError("No program specified")
print("Trying to run program: ", program)

############################################################ CPU and Cache Setup
system = System()


# Create a top-level voltage domain
system.voltage_domain = VoltageDomain(voltage = args.sys_voltage)
system.clk_domain = SrcClockDomain(clock =  args.sys_clock,
    voltage_domain = system.voltage_domain)


system.cpu_voltage_domain = VoltageDomain()
system.cpu_clk_domain = SrcClockDomain(clock = args.cpu_clock,
                                       voltage_domain =
                                       system.cpu_voltage_domain)

system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('512MB')]

system.cache_line_size = 64

system.cpu = RiscvO3CPU(cpu_id = 0)
system.acc = FedexCentral()
# system.cpu = [RiscvO3CPU(cpu_id = 0), RiscvTimingSimpleCPU(cpu_id = 1)]


# All cpus belong to a common cpu_clk_domain, therefore running at a common
# frequency.
for cpu in system.cpu:
    cpu.clk_domain = system.cpu_clk_domain

########################################################### Setup Program
def setup_program(program, idx = 0):
    print("Setting Program to run: ", program)
    process = Process(pid = 100 + idx)
    process.executable = program
    process.cwd = os.getcwd()
    process.gid = os.getgid()
    process.cmd = [program]

    return process

process = setup_program(program)
system.cpu.workload = process
system.cpu.createThreads()

#### PlaceHolder for now
# system.cpu[1].workload = process
# system.cpu[1].createThreads()

########################################################### Mem and Cache Setup

### Override Argument Cache Setup

args.caches=True
args.l2cache=True

args.l1i_size = '32kB'
args.l1d_size = '64kB'
args.l1i_assoc = 8
args.l1d_assoc = 8

args.l2_size = '2MB'
args.l2_assoc = 16

system.membus = SystemXBar()
system.system_port = system.membus.cpu_side_ports

#### Cache Setup
CacheConfig.config_cache(args, system)
# system.cpu[1].createInterruptController()
# system.cpu[1].dcache_port = system.membus.cpu_side_ports

MemConfig.config_mem(args, system)
config_filesystem(system, args)

########################################################### Fedex Connection
system.acc.o3RequestPort = system.cpu.to_fedex_central
system.tol2bus.cpu_side_ports = system.acc.dataPort

########################################################### Run Simulation
system.workload = SEWorkload.init_compatible(program)
root = Root(full_system = False, system = system)

#### m5 will find root by itself
m5.instantiate()
m5.simulate()


