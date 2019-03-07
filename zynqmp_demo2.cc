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
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/tlm_quantumkeeper.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "trace.h"
#include "iconnect.h"
#include "debugdev.h"
#include "xilinx-zynqmp2.h"

#define NR_MASTERS	1
#define NR_DEVICES	1

#include "tlm_utils/simple_target_socket.h"

/**
 * Example of a simple counter using TLM
 */
class ecounter : public sc_core::sc_module
{
public:
   virtual void b_transport(tlm::tlm_generic_payload& transaction, sc_time& delay);
   public:
      tlm_utils::simple_target_socket<ecounter> t_sk;
      sc_out<bool> irq;

      ecounter(sc_module_name name)
      : sc_module(name), t_sk("target-socket")
      {
         t_sk.register_b_transport(this, &ecounter::b_transport);
	     m_count = 0;
      }
      SC_HAS_PROCESS(ecounter);
   private:
      unsigned int m_count;
};
void ecounter::b_transport(tlm::tlm_generic_payload& transaction, sc_time& delay)
{
	tlm::tlm_command cmd = transaction.get_command();
	unsigned char *data = transaction.get_data_ptr();
	tlm::tlm_response_status status = tlm::TLM_OK_RESPONSE;

	switch(cmd)
	{
		case (tlm::TLM_READ_COMMAND):
			m_count = m_count + 1;
			data[0] = m_count & 0xFF;
			data[1] = (m_count>>8) & 0xFF;
			data[2] = (m_count>>16) & 0xFF;
			data[3] = (m_count>>24) & 0xFF;
			if(m_count == 9)
			{
				irq.write(true);
				wait(sc_time(1, SC_US));
				irq.write(false);
			}
			break;
		case (tlm::TLM_WRITE_COMMAND):
			m_count = 0;
			break;
		default:
			status = tlm::TLM_COMMAND_ERROR_RESPONSE;
	}

	transaction.set_response_status(status);
}

SC_MODULE(Top)
{
	SC_HAS_PROCESS(Top);
	xilinx_zynqmp zynq;
	ecounter *counter;
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

		counter =  new ecounter("Counter");
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
	sc_trace_file *trace_fp = NULL;

	if (argc < 3) {
		sync_quantum = 10000;
	} else {
		sync_quantum = strtoull(argv[2], NULL, 10);
	}

	sc_set_time_resolution(1, SC_PS);

	top = new Top("top", argv[1], sc_time((double) sync_quantum, SC_NS));

	if (argc < 3) {
		sc_start(1, SC_PS);
		sc_stop();
		usage();
		exit(EXIT_FAILURE);
	}

	trace_fp = sc_create_vcd_trace_file("trace");
	trace(trace_fp, *top, top->name());
	/* Pull the reset signal.  */
	top->rst.write(true);
	sc_start(1, SC_US);
	top->rst.write(false);

	sc_start();
	if (trace_fp) {
		sc_close_vcd_trace_file(trace_fp);
	}
	return 0;
}
