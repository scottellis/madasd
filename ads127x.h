/*
 * GPIO pin use for control of the ads127x module using HTG's custom Overo
 * expansion board.
 *
 * All GPIO pins muxed as IDIS | PTD | EN | M4 - input disabled, pulled-down.
 *
 */

#ifndef ADS127X_H
#define ADS127X_H

/* GPIO pin mappings to ADS127x control pins */
#define GPIO_PWDN1	71
#define GPIO_PWDN2	70
#define GPIO_PWDN3	73
#define GPIO_PWDN4	75
#define GPIO_PWDN5	72
#define GPIO_PWDN6	74
#define GPIO_PWDN7	10
#define GPIO_PWDN8	80

#define GPIO_MODE0	81
#define GPIO_MODE1	186

#define GPIO_CLKDIV	68

#define GPIO_FORMAT0	87
#define GPIO_FORMAT1	91
#define	GPIO_FORMAT2	92

#define	GPIO_SYNC	67

/* #define FSYNC	MCBSP3_FSC,GPIO_147,UART2_RX */
/* #define SCLK		MCBSP3_CLKX,GPIO_146,UART2_TX */
/* #define DOUT1	MCBSP3_DR,GPIO_145,UART2_RTS */

/* only one input line DR (DOUT1) is used
#define DOUT2	88	* DSS_DATA18 *
#define DOUT3	89	* DSS_DATA19 *
#define DOUT4	79	* DSS_DATA9 *
#define DOUT5	77	* DSS_DATA7 *
#define DOUT6	78	* DSS_DATA8 *
#define DOUT7	66	* DSS_PCLK *
#define DOUT8	76	* DSS_DATA6 *
*/


/* IOCTL constants */

#define ADS127X_IOC_MAGIC		0xaa

// McBSP source clock frequency
#define ADS127X_IOC_GET_CLK_FREQ	_IOR(ADS127X_IOC_MAGIC, 1, int)


/* the range is 0-255 */
#define ADS127X_IOC_GET_MCBSP_CLKDIV	_IOR(ADS127X_IOC_MAGIC, 2, int)
#define ADS127X_IOC_SET_MCBSP_CLKDIV	_IOW(ADS127X_IOC_MAGIC, 3, int)

#define MIN_HIGH_SPEED_CLKDIV 	3
#define MIN_HIGH_RES_CLKDIV	4
#define MIN_LOW_SPEED_CLKDIV_1	4
#define MIN_LOW_SPEED_CLKDIV_0	8
#define MIN_LOW_POWER_CLKDIV_1	4
#define MIN_LOW_POWER_CLKDIV_0	18


#define MODE_HIGH_SPEED		0
#define MODE_HIGH_RESOLUTION	1
#define MODE_LOW_SPEED		2
#define MODE_LOW_POWER		3

#define ADS127X_IOC_GET_MODE		_IOR(ADS127X_IOC_MAGIC, 4, int)
#define ADS127X_IOC_SET_MODE		_IOW(ADS127X_IOC_MAGIC, 5, int)


#define FORMAT_FS_TDM_DYNAMIC	0
#define FORMAT_FS_TDM_FIXED	1

#define ADS127X_IOC_GET_FORMAT		_IOR(ADS127X_IOC_MAGIC, 6, int)
#define ADS127X_IOC_SET_FORMAT		_IOW(ADS127X_IOC_MAGIC, 7, int)

#define ADS127X_IOC_GET_ADS_CLKDIV	_IOR(ADS127X_IOC_MAGIC, 8, int)
#define ADS127X_IOC_SET_ADS_CLKDIV	_IOW(ADS127X_IOC_MAGIC, 9, int)


#define CH1	(1 << 0)
#define CH2	(1 << 1)
#define CH3	(1 << 2)
#define CH4	(1 << 3)
#define CH5	(1 << 4)
#define CH6	(1 << 5)
#define CH7	(1 << 6)
#define CH8	(1 << 7)

/* this is a bit field of the above CHx defs */
#define ADS127X_IOC_GET_CHANNELS	_IOR(ADS127X_IOC_MAGIC, 10, int)
#define ADS127X_IOC_SET_CHANNELS	_IOW(ADS127X_IOC_MAGIC, 11, int)

#define ADS127X_IOC_GET_NUM_CHANNELS		_IOR(ADS127X_IOC_MAGIC, 12, int)
#define ADS127X_IOC_GET_FRAMES_PER_BLOCK	_IOR(ADS127X_IOC_MAGIC, 13, int)
#define ADS127X_IOC_GET_BYTES_PER_BLOCK		_IOR(ADS127X_IOC_MAGIC, 14, int)
#define ADS127X_IOC_GET_SAMPLE_RATE		_IOR(ADS127X_IOC_MAGIC, 15, int)

// driver state flags
#define MCBSP_ACQUIRED		(1 << 0)
#define MCBSP_CONFIG_SET	(1 << 1)
#define MCBSP_RUNNING		(1 << 2)
#define USER_STALLED		(1 << 3)
#define ADS_CONFIG_SET		(1 << 4)


#define ADS127X_IOC_GET_STATE			_IOR(ADS127X_IOC_MAGIC, 20, int)

struct ts_correlation
{
#ifdef __KERNEL__
        u32 tod_tv_sec;
	u32 tod_tv_usec;
	u64 cpu_clock;
#else
	uint32_t tod_tv_sec;
	uint32_t tod_tv_usec;
	uint64_t cpu_clock;
#endif
};

#define ADS127X_IOC_GET_TS_CORRELATION		_IOR(ADS127X_IOC_MAGIC, 25, struct ts_correlation)

#endif /* ifndef ADS127X_H */


