#define SC_INCLUDE_DYNAMIC_PROCESSES

#include <inttypes.h>

#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

using namespace sc_core;
using namespace std;

#include "sample-counter.h"
#include <sys/types.h>

sCounter::sCounter(sc_module_name name)
	: sc_module(name), t_sk("target-socket")
{
	t_sk.register_b_transport(this, &sCounter::b_transport);
	m_count = 0;
	m_ticksCount = 0;

	m_output_freq = COUNTER_OUTPUT_FREQ;
	m_ticksPerCount = INPUT_CLK_FREQ/m_output_freq;
	m_ctrl = COUNTER_CTRL_RESET;
	m_state = COUNTER_STATE_RESET;

	SC_METHOD(execute);
	sensitive << clk.pos();
}

void sCounter::execute(void)
{
	uint8_t next = m_state;
	switch(m_state)
	{
	case COUNTER_STATE_RESET:
		if(m_ctrl == COUNTER_CTRL_ENABLE)
		{
			next = COUNTER_STATE_COUNTING;
		}
		break;
	case COUNTER_STATE_COUNTING:
		if(m_ctrl == COUNTER_CTRL_RESET)
		{
			next = COUNTER_STATE_RESET;
			m_count = 0;
			m_ticksCount = 0;
		}
		else
		{
			if(m_ticksCount < m_ticksPerCount)
			{
				m_ticksCount++;
			}
			else
			{
				m_count++;
				m_ticksCount = 0;
				//irq.write(true);
				//wait(sc_time(20, SC_NS));
				//irq.write(false);
			}
		}

		break;
	default:
		break;
	}
	m_state = next;
}

void sCounter::b_transport(tlm::tlm_generic_payload& transaction, sc_time& delay)
{
	tlm::tlm_command cmd = transaction.get_command();
	sc_dt::uint64 addr = transaction.get_address();
	unsigned char *data = transaction.get_data_ptr();
	tlm::tlm_response_status status = tlm::TLM_OK_RESPONSE;

	switch(cmd)
	{
		case (tlm::TLM_READ_COMMAND):
			if(addr == COUNTER_REGISTER_COUNT)
			{
				memcpy(data, &m_count, 4);
			}
			else
			{
				status = tlm::TLM_ADDRESS_ERROR_RESPONSE;
			}
			break;
		case (tlm::TLM_WRITE_COMMAND):
			if(addr == COUNTER_REGISTER_CTRL)
			{
				memcpy(&m_ctrl, data, 4);
			}
			else if (addr == COUNTER_REGISTER_OUTPUT_FREQ)
			{
				memcpy(&m_output_freq, data, 4);
				m_ticksPerCount = INPUT_CLK_FREQ/m_output_freq;
			}
			else
			{
				status = tlm::TLM_ADDRESS_ERROR_RESPONSE;
			}
			break;
		default:
			status = tlm::TLM_COMMAND_ERROR_RESPONSE;
	}
	transaction.set_response_status(status);
}

