#ifndef _ALTERA_HPS_H_
#define _ALTERA_HPS_H_

/*
 * This file was automatically generated by the swinfo2header utility.
 * 
 * Created from SOPC Builder system 'soc_system' in
 * file './soc_system.sopcinfo'.
 */

/*
 * This file contains macros for module 'hps_0' and devices
 * connected to the following masters:
 *   h2f_axi_master
 *   h2f_lw_axi_master
 * 
 * Do not include this header file and another header file created for a
 * different module or master group at the same time.
 * Doing so may result in duplicate macro names.
 * Instead, use the system header file which has macros with unique names.
 */

/*
 * Macros for device 'fifo_to_copro_in', class 'altera_avalon_fifo'
 * The macros are prefixed with 'FIFO_TO_COPRO_IN_'.
 * The prefix is the slave descriptor.
 */
#define FIFO_TO_COPRO_IN_COMPONENT_TYPE altera_avalon_fifo
#define FIFO_TO_COPRO_IN_COMPONENT_NAME fifo_to_copro
#define FIFO_TO_COPRO_IN_BASE 0x0
#define FIFO_TO_COPRO_IN_SPAN 4
#define FIFO_TO_COPRO_IN_END 0x3
#define FIFO_TO_COPRO_IN_AVALONMM_AVALONMM_DATA_WIDTH 32
#define FIFO_TO_COPRO_IN_AVALONMM_AVALONST_DATA_WIDTH 32
#define FIFO_TO_COPRO_IN_BITS_PER_SYMBOL 16
#define FIFO_TO_COPRO_IN_CHANNEL_WIDTH 8
#define FIFO_TO_COPRO_IN_ERROR_WIDTH 8
#define FIFO_TO_COPRO_IN_FIFO_DEPTH 256
#define FIFO_TO_COPRO_IN_SINGLE_CLOCK_MODE 1
#define FIFO_TO_COPRO_IN_SYMBOLS_PER_BEAT 2
#define FIFO_TO_COPRO_IN_USE_AVALONMM_READ_SLAVE 1
#define FIFO_TO_COPRO_IN_USE_AVALONMM_WRITE_SLAVE 1
#define FIFO_TO_COPRO_IN_USE_AVALONST_SINK 0
#define FIFO_TO_COPRO_IN_USE_AVALONST_SOURCE 0
#define FIFO_TO_COPRO_IN_USE_BACKPRESSURE 1
#define FIFO_TO_COPRO_IN_USE_IRQ 0
#define FIFO_TO_COPRO_IN_USE_PACKET 1
#define FIFO_TO_COPRO_IN_USE_READ_CONTROL 0
#define FIFO_TO_COPRO_IN_USE_REGISTER 0
#define FIFO_TO_COPRO_IN_USE_WRITE_CONTROL 1

/*
 * Macros for device 'fifo_to_copro_in_csr', class 'altera_avalon_fifo'
 * The macros are prefixed with 'FIFO_TO_COPRO_IN_CSR_'.
 * The prefix is the slave descriptor.
 */
#define FIFO_TO_COPRO_IN_CSR_COMPONENT_TYPE altera_avalon_fifo
#define FIFO_TO_COPRO_IN_CSR_COMPONENT_NAME fifo_to_copro
#define FIFO_TO_COPRO_IN_CSR_BASE 0x20
#define FIFO_TO_COPRO_IN_CSR_SPAN 32
#define FIFO_TO_COPRO_IN_CSR_END 0x3f
#define FIFO_TO_COPRO_IN_CSR_AVALONMM_AVALONMM_DATA_WIDTH 32
#define FIFO_TO_COPRO_IN_CSR_AVALONMM_AVALONST_DATA_WIDTH 32
#define FIFO_TO_COPRO_IN_CSR_BITS_PER_SYMBOL 16
#define FIFO_TO_COPRO_IN_CSR_CHANNEL_WIDTH 8
#define FIFO_TO_COPRO_IN_CSR_ERROR_WIDTH 8
#define FIFO_TO_COPRO_IN_CSR_FIFO_DEPTH 256
#define FIFO_TO_COPRO_IN_CSR_SINGLE_CLOCK_MODE 1
#define FIFO_TO_COPRO_IN_CSR_SYMBOLS_PER_BEAT 2
#define FIFO_TO_COPRO_IN_CSR_USE_AVALONMM_READ_SLAVE 1
#define FIFO_TO_COPRO_IN_CSR_USE_AVALONMM_WRITE_SLAVE 1
#define FIFO_TO_COPRO_IN_CSR_USE_AVALONST_SINK 0
#define FIFO_TO_COPRO_IN_CSR_USE_AVALONST_SOURCE 0
#define FIFO_TO_COPRO_IN_CSR_USE_BACKPRESSURE 1
#define FIFO_TO_COPRO_IN_CSR_USE_IRQ 0
#define FIFO_TO_COPRO_IN_CSR_USE_PACKET 1
#define FIFO_TO_COPRO_IN_CSR_USE_READ_CONTROL 0
#define FIFO_TO_COPRO_IN_CSR_USE_REGISTER 0
#define FIFO_TO_COPRO_IN_CSR_USE_WRITE_CONTROL 1

/*
 * Macros for device 'fifo_to_hps_out', class 'altera_avalon_fifo'
 * The macros are prefixed with 'FIFO_TO_HPS_OUT_'.
 * The prefix is the slave descriptor.
 */
#define FIFO_TO_HPS_OUT_COMPONENT_TYPE altera_avalon_fifo
#define FIFO_TO_HPS_OUT_COMPONENT_NAME fifo_to_hps
#define FIFO_TO_HPS_OUT_BASE 0x40
#define FIFO_TO_HPS_OUT_SPAN 4
#define FIFO_TO_HPS_OUT_END 0x43
#define FIFO_TO_HPS_OUT_AVALONMM_AVALONMM_DATA_WIDTH 32
#define FIFO_TO_HPS_OUT_AVALONMM_AVALONST_DATA_WIDTH 32
#define FIFO_TO_HPS_OUT_BITS_PER_SYMBOL 16
#define FIFO_TO_HPS_OUT_CHANNEL_WIDTH 8
#define FIFO_TO_HPS_OUT_ERROR_WIDTH 8
#define FIFO_TO_HPS_OUT_FIFO_DEPTH 256
#define FIFO_TO_HPS_OUT_SINGLE_CLOCK_MODE 0
#define FIFO_TO_HPS_OUT_SYMBOLS_PER_BEAT 2
#define FIFO_TO_HPS_OUT_USE_AVALONMM_READ_SLAVE 1
#define FIFO_TO_HPS_OUT_USE_AVALONMM_WRITE_SLAVE 1
#define FIFO_TO_HPS_OUT_USE_AVALONST_SINK 0
#define FIFO_TO_HPS_OUT_USE_AVALONST_SOURCE 0
#define FIFO_TO_HPS_OUT_USE_BACKPRESSURE 1
#define FIFO_TO_HPS_OUT_USE_IRQ 0
#define FIFO_TO_HPS_OUT_USE_PACKET 1
#define FIFO_TO_HPS_OUT_USE_READ_CONTROL 1
#define FIFO_TO_HPS_OUT_USE_REGISTER 0
#define FIFO_TO_HPS_OUT_USE_WRITE_CONTROL 0

/*
 * Macros for device 'fifo_to_hps_out_csr', class 'altera_avalon_fifo'
 * The macros are prefixed with 'FIFO_TO_HPS_OUT_CSR_'.
 * The prefix is the slave descriptor.
 */
#define FIFO_TO_HPS_OUT_CSR_COMPONENT_TYPE altera_avalon_fifo
#define FIFO_TO_HPS_OUT_CSR_COMPONENT_NAME fifo_to_hps
#define FIFO_TO_HPS_OUT_CSR_BASE 0x60
#define FIFO_TO_HPS_OUT_CSR_SPAN 32
#define FIFO_TO_HPS_OUT_CSR_END 0x7f
#define FIFO_TO_HPS_OUT_CSR_AVALONMM_AVALONMM_DATA_WIDTH 32
#define FIFO_TO_HPS_OUT_CSR_AVALONMM_AVALONST_DATA_WIDTH 32
#define FIFO_TO_HPS_OUT_CSR_BITS_PER_SYMBOL 16
#define FIFO_TO_HPS_OUT_CSR_CHANNEL_WIDTH 8
#define FIFO_TO_HPS_OUT_CSR_ERROR_WIDTH 8
#define FIFO_TO_HPS_OUT_CSR_FIFO_DEPTH 256
#define FIFO_TO_HPS_OUT_CSR_SINGLE_CLOCK_MODE 0
#define FIFO_TO_HPS_OUT_CSR_SYMBOLS_PER_BEAT 2
#define FIFO_TO_HPS_OUT_CSR_USE_AVALONMM_READ_SLAVE 1
#define FIFO_TO_HPS_OUT_CSR_USE_AVALONMM_WRITE_SLAVE 1
#define FIFO_TO_HPS_OUT_CSR_USE_AVALONST_SINK 0
#define FIFO_TO_HPS_OUT_CSR_USE_AVALONST_SOURCE 0
#define FIFO_TO_HPS_OUT_CSR_USE_BACKPRESSURE 1
#define FIFO_TO_HPS_OUT_CSR_USE_IRQ 0
#define FIFO_TO_HPS_OUT_CSR_USE_PACKET 1
#define FIFO_TO_HPS_OUT_CSR_USE_READ_CONTROL 1
#define FIFO_TO_HPS_OUT_CSR_USE_REGISTER 0
#define FIFO_TO_HPS_OUT_CSR_USE_WRITE_CONTROL 0

/*
 * Macros for device 'pio_0', class 'altera_avalon_pio'
 * The macros are prefixed with 'PIO_0_'.
 * The prefix is the slave descriptor.
 */
#define PIO_0_COMPONENT_TYPE altera_avalon_pio
#define PIO_0_COMPONENT_NAME pio_0
#define PIO_0_BASE 0x1000
#define PIO_0_SPAN 16
#define PIO_0_END 0x100f
#define PIO_0_BIT_CLEARING_EDGE_REGISTER 0
#define PIO_0_BIT_MODIFYING_OUTPUT_REGISTER 0
#define PIO_0_CAPTURE 0
#define PIO_0_DATA_WIDTH 32
#define PIO_0_DO_TEST_BENCH_WIRING 0
#define PIO_0_DRIVEN_SIM_VALUE 0
#define PIO_0_EDGE_TYPE NONE
#define PIO_0_FREQ 50000000
#define PIO_0_HAS_IN 1
#define PIO_0_HAS_OUT 0
#define PIO_0_HAS_TRI 0
#define PIO_0_IRQ_TYPE NONE
#define PIO_0_RESET_VALUE 0


#endif /* _ALTERA_HPS_H_ */