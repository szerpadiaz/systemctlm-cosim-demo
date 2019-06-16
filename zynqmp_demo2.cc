/*
 * Top level of the ZynqMP cosim example.
 *
 * Copyright (c) 2014 Xilinx Inc.
 * Written by Edgar E. Iglesias
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include <inttypes.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "systemc.h"
#include "tlm_utils/tlm_quantumkeeper.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "xilinx-zynqmp2.h"
#include "sample-counter.h"

SC_MODULE(Top)
{
	SC_HAS_PROCESS(Top);
	xilinx_zynqmp zynq;
	sc_clock *clk;
	sCounter *counter;
	sc_signal<bool> rst, rst_n;

	void gen_rst_n(void)
	{
		rst_n.write(!rst.read());
	}

	Top(sc_module_name name, const char *sk_descr, sc_time quantum) :
		zynq("zynq", sk_descr),
		rst("rst"),
		rst_n("rst_n")
	{
		SC_METHOD(gen_rst_n);
		sensitive << rst;

		m_qk.set_global_quantum(quantum);

		zynq.rst(rst);

		clk = new sc_clock("clk", sc_time(20, SC_NS));
		counter =  new sCounter("Counter");
		counter->clk(*clk);
		zynq.s_data->bind(counter->t_sk);
		counter->irq(zynq.pl2ps_irq[1]);

		zynq.tie_off();
	}

private:
	tlm_utils::tlm_quantumkeeper m_qk;
};

void usage(void)
{
	cout << "tlm socket-path sync-quantum-ns" << endl;
}

int sc_main(int argc, char* argv[])
{
	Top *top;
	uint64_t sync_quantum;

	if (argc < 3) {
		sync_quantum = 10000;
	} else {
		sync_quantum = strtoull(argv[2], NULL, 10);
	}

	// sc_set_time_resolution can only be called before the first
	// sc_time object is created. This means that after setting the
	// global quantum it will not be possible to call sc_set_time_resolution.
	// If sc_set_time_resolution must be called this must be done before
	// the global quantum is set.
	sc_set_time_resolution(1, SC_PS);

	top = new Top("top", argv[1], sc_time((double) sync_quantum, SC_NS));

	if (argc < 3) {
		sc_start(1, SC_PS);
		sc_stop();
		usage();
		exit(EXIT_FAILURE);
	}

	/* Pull the reset signal.  */
	top->rst.write(true);
	sc_start(1, SC_US);
	top->rst.write(false);

	sc_start();
	return 0;
}
