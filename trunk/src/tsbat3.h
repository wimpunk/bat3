/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
 
 struct BAT3request {
	unsigned alarm;
	union {
		struct {
			unsigned short PWM1en:1,
				       PWM2en:1,
				       _offsetEn:1,
				       opampEn:1,
				       _buckEn:1,
				       _led:1,
				       _jp3:1,
				       batRun:1,
				       ee_read:1,
				       ee_write:1,
				       ee_ready:1,
				       softJP3:1;
		} __attribute__((packed));
		unsigned short outputs;
	} __attribute__((packed));
	unsigned short pwm_lo; // low time
	unsigned short pwm_t; // high time plus low time
	unsigned char ee_addr;
	unsigned char ee_data;
	unsigned char checksum;

} __attribute__((packed));

struct BAT3reply {
	unsigned short adc0; // input supply voltage
	unsigned short adc2; // regulated supply voltage
	unsigned short adc6; // battery voltage
	unsigned short adc7; // battery current
	unsigned short pwm_lo; // high time
	unsigned short pwm_t; // high time plus low time
	unsigned short temp; // temperature in TMP124 format
	union {
		struct {
			unsigned short PWM1en:1,
				       PWM2en:1,
				       _offsetEn:1,
				       opampEn:1,
				       _buckEn:1,
				       _led:1,
				       _jp3:1,
				       batRun:1,
				       ee_read:1,
				       ee_write:1,
				       ee_ready:1,
				       softJP3:1;
		} __attribute__((packed));
		unsigned short outputs;
	} __attribute__((packed));
	unsigned char ee_addr;
	unsigned char ee_data;
	unsigned char checksum;
} __attribute__((packed));

