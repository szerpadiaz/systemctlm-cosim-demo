
#define COUNTER_REGISTER_CTRL  		  (0xFF4E0000)
#define COUNTER_REGISTER_OUTPUT_FREQ  (0xFF4E0004)
#define COUNTER_REGISTER_COUNT 		  (0xFF4E0008)
#define COUNTER_CTRL_RESET     (0)
#define COUNTER_CTRL_ENABLE    (1)
#define COUNTER_CTRL_IRQ_EN    (2)

#define COUNTER_MAX_COUNT 	   (9)
#define COUNTER_STATE_RESET    (0)
#define COUNTER_STATE_COUNTING (1)

#define INPUT_CLK_FREQ         (50000000)
#define COUNTER_OUTPUT_FREQ    (50000000)

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
	 * input clock
	 */
	sc_in_clk clk;

	/**
	 * Interrupt signal as sc-output
	 */
	sc_out<bool> irq;

	sCounter(sc_core::sc_module_name name);
	SC_HAS_PROCESS(sCounter);

private:
	uint32_t m_count;
	uint32_t m_ticksCount;

	uint32_t m_ctrl;
	uint32_t m_output_freq;
	uint32_t m_ticksPerCount;
	uint8_t m_state;

	void execute(void);
	virtual void b_transport(tlm::tlm_generic_payload& transaction, sc_time& delay);
};
