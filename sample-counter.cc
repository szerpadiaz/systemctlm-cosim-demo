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
	m_ctrl = COUNTER_CTRL_RESET;
	m_state = COUNTER_STATE_RESET;

	//SC_THREAD(execute);
	//dont_initialize();
	//sensitive << m_event_from_socket;
}

void sCounter::execute(void)
{
	uint8_t next;
	//while(true)
	{
		//wait(m_event_from_socket);
		next = m_state;
		switch(m_state)
		{
			case COUNTER_STATE_RESET:
				if(m_ctrl == COUNTER_CTRL_ENABLE)
				{
					next = COUNTER_STATE_COUNTING;
				}
				break;
			case COUNTER_STATE_COUNTING:
				if(m_ctrl == COUNTER_CTRL_INCREMENT)
				{
					if(m_count < COUNTER_MAX_COUNT)
					{
						m_count++;
					}
					else
					{
						next = COUNTER_STATE_RESET;
						m_count = 0;

						irq.write(true);
						wait(sc_time(5, SC_NS));
						irq.write(false);
					}
				}
				else if(m_ctrl == COUNTER_CTRL_RESET)
				{
					next = COUNTER_STATE_RESET;
					m_count = 0;
				}
				break;
			default:
				break;
		}
		m_state = next;
	}
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
				m_ctrl = COUNTER_CTRL_INCREMENT;
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
			else
			{
				status = tlm::TLM_ADDRESS_ERROR_RESPONSE;
			}
			break;
		default:
			status = tlm::TLM_COMMAND_ERROR_RESPONSE;
	}
	execute();
	//m_event_from_socket.notify();
	transaction.set_response_status(status);
}

