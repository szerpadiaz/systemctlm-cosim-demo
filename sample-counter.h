
#define COUNTER_REGISTER_CTRL  (0xFF4E0000)
#define COUNTER_REGISTER_COUNT (0xFF4E0004)
#define COUNTER_CTRL_RESET     (0)
#define COUNTER_CTRL_ENABLE    (1)
#define COUNTER_CTRL_INCREMENT (2)

#define COUNTER_MAX_COUNT 	   (9)
#define COUNTER_STATE_RESET    (0)
#define COUNTER_STATE_COUNTING (1)

/**
 * Sample counter using TLM-sockets as input
 */
class sCounter
: public sc_core::sc_module
{
public:
	/**
	 * Target socket to get the data coming from the PS.
	 */
	tlm_utils::simple_target_socket<sCounter> t_sk;

	/**
	 * Interrupt signal as sc-output
	 */
	sc_out<bool> irq;

	sCounter(sc_core::sc_module_name name);
	SC_HAS_PROCESS(sCounter);

private:
	//sc_event m_event_from_socket;
	uint32_t m_ctrl;
	uint32_t m_count;
	uint8_t m_state;

	void execute(void);
	virtual void b_transport(tlm::tlm_generic_payload& transaction, sc_time& delay);
};
