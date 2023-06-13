/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
	TODO:
		at this point:
			- complete the set stereo_mode verb.
			- find a way to tell the service which i2c chanel is used.
			- separate the functions of driver from the verbs by creating new c file.
			- find a way of monitoring the quality of tuning and correct it time by time.
			- use Interupt for getting RDS data
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdarg.h>
#include <error.h>
#include <gst/gst.h>
#include <time.h>
#include <gst/gst.h>
#include <pthread.h>

#include "radio_impl.h"
#include "tef665x.h"

#define I2C_ADDRESS 0x64
#define I2C_DEV "/dev/i2c-3"
#define VERSION "0.1"

#define TEF665x_CMD_LEN_MAX	20
#define SET_SUCCESS 1
#define TEF665X_SPLIT_SIZE		24

#define TEF665x_REF_CLK		9216000	//reference clock frequency
#define TEF665x_IS_CRYSTAL_CLK	0	//crstal
#define TEF665x_IS_EXT_CLK	1	//external clock input

#define High_16bto8b(a)	  ((uint8_t)((a) >> 8))
#define Low_16bto8b(a) 	  ((uint8_t)(a))
#define Convert8bto16b(a) ((uint16_t)(((uint16_t)(*(a))) << 8 |((uint16_t)(*(a+1)))))

#define GST_PIPELINE_LEN    256

const uint8_t tef665x_patch_cmdTab1[] = {3,	0x1c,0x00,0x00};
const uint8_t tef665x_patch_cmdTab2[] = {3,	0x1c,0x00,0x74};
const uint8_t tef665x_patch_cmdTab3[] = {3,	0x1c,0x00,0x75};


typedef struct {
	char *name;
	uint32_t min;
	uint32_t max;
	uint32_t step;
} band_plan_t;

typedef struct{
	radio_scan_callback_t callback;
	radio_scan_direction_t direction;
	void* data;
}scan_data_t;
typedef struct rds_data
{
	bool Text_Changed; 
	bool TrafficAnnouncement;
	bool TrafficProgram;
	bool Music_Speech;

	uint32_t Alternative_Freq[25];
	uint8_t  Alternative_Freq_Counter;
	uint8_t  Num_AlterFreq;

	uint16_t PICode;	
	uint8_t  DI_Seg;
	uint8_t  PTY_Code;

	uint8_t Year;
	uint8_t Month;
	uint8_t Day;
	uint8_t Hour;
	uint8_t Min;

	uint8_t PTYN_Size;
	uint8_t raw_data[12];

	char PS_Name[16];
	char RT[128];
	char PTYN[16];
} rds_data_t;

//thread for handling RDS and Mutex
pthread_t  rds_thread;
rds_data_t RDS_Message;
pthread_mutex_t RDS_Mutex;

station_quality_t quality;

//Threads for handling Scan
pthread_t scan_thread;

pthread_mutex_t scan_mutex;

char _Temp[64]={0};

static band_plan_t known_fm_band_plans[5] = {
	{ .name = "US", .min = 87900000, .max = 107900000, .step = 200000 },
	{ .name = "JP", .min = 76000000, .max = 95000000, .step = 100000 },
	{ .name = "EU", .min = 87500000, .max = 108000000, .step = 50000 },
	{ .name = "ITU-1", .min = 87500000, .max = 108000000, .step = 50000 },
	{ .name = "ITU-2", .min = 87900000, .max = 107900000, .step = 50000 }
};
static band_plan_t known_am_band_plans[1] = {
	{ .name = "W-ASIA", .min = 522000, .max = 1620000, .step = 9000 }
};

static unsigned int fm_bandplan = 2;
static unsigned int am_bandplan = 0;
static bool corking;
static bool present;
static bool initialized;
static bool scanning;

// stream state
static GstElement *pipeline;
static bool running;


uint32_t AlterFreqOffset=0;

static void (*freq_callback)(uint32_t, void*);
static void (*rds_callback) (void*);
static void *freq_callback_data;

int  tef665x_set_rds    (uint32_t i2c_file_desc);
#define DEBUG 0

#if DEBUG == 1
#define _debug(x, y) printf("function: %s,  %s : %d\n", __FUNCTION__, #x, y)
#else
#define _debug(x, y)
#endif

static uint32_t file_desc;

static radio_band_t current_band;
static uint32_t current_am_frequency;
static uint32_t current_fm_frequency;

static void tef665x_scan_stop        (void);
static void tef665x_set_frequency    (uint32_t);
static void tef665x_search_frequency (uint32_t);

static uint32_t tef665x_get_min_frequency  (radio_band_t);
static uint32_t tef665x_get_max_frequency  (radio_band_t);
static uint32_t tef665x_get_frequency_step (radio_band_t);

static station_quality_t *tef665x_get_quality_info (void);

static gboolean handle_message(GstBus *bus, GstMessage *msg, __attribute__((unused)) void *ptr)
{
	GstState state;

	if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_REQUEST_STATE) {

		gst_message_parse_request_state(msg, &state);

		if (state == GST_STATE_PAUSED)
			corking = true;
		else if (state == GST_STATE_PLAYING)
			corking = false;

	}

	return TRUE;
}

static int tef665x_set_cmd(int i2c_file_desc, TEF665x_MODULE module, uint8_t cmd, int len, ...)
{
	int i, ret;
	uint8_t buf[TEF665x_CMD_LEN_MAX];
	uint16_t temp;
    va_list vArgs;

    va_start(vArgs, len);

	buf[0] = module;	//module,	FM/AM/APP
	buf[1] = cmd;		//cmd,		1,2,10,...
	buf[2] = 0x01;	    //index, 	always 1

	for(i = 3; i < len; i++)
	{
		temp = va_arg(vArgs,int);

		buf[i++] = High_16bto8b(temp);
		buf[i] = Low_16bto8b(temp);
	}

	va_end(vArgs);

	ret = write(i2c_file_desc, buf, len);

	temp = (ret == len) ? 1 : 0;
	_debug("return value", temp);
	return temp;
}

static int tef665x_get_cmd(int i2c_file_desc, TEF665x_MODULE module, uint8_t cmd, uint8_t *receive, int len)
{
	uint8_t temp;
	uint8_t buf[3];
	int ret;

	buf[0]= module;		//module,	FM/AM/APP
	buf[1]= cmd;		//cmd,		1,2,10,...
	buf[2]= 1;	        //index, 	always 1

	write(i2c_file_desc, buf, 3);

	ret = read(i2c_file_desc, receive, len);
	temp = (ret == len) ? 1 : 0;
	_debug("return value", temp);
	if(temp==0)
		fprintf(stderr, "Error Number: %d: %s",errno,strerror(errno));
	return temp;
}

/*
module 64 APPL
cmd 128 Get_Operation_Status | status
index
1 status
	Device operation status
	0 = boot state; no command support
	1 = idle state
	2 = active state; radio standby
	3 = active state; FM
	4 = active state; AM
*/
static int appl_get_operation_status(int i2c_file_desc ,uint8_t *status)
{
   	uint8_t buf[2];
	int ret;

    ret = tef665x_get_cmd(i2c_file_desc, TEF665X_MODULE_APPL,
			TEF665X_Cmd_Get_Operation_Status,
			buf, sizeof(buf));

	if(ret == SET_SUCCESS)
	{
		*status = Convert8bto16b(buf);
		_debug("return value", 1);
		return 1;
	}
	_debug("return value", 0);
	return 0;
}

static int get_operation_status(int i2c_file_desc, TEF665x_STATE *status)
{
	TEF665x_STATE data;
	int ret;
	if(SET_SUCCESS ==(ret = appl_get_operation_status(i2c_file_desc, &data)))
	{
		//printk( "appl_get_operation_status1 data= %d \n",data);
		_debug("got status", ret);
		switch(data)
		{
			case 0:
				_debug("status: boot", ret);
				*status = eDevTEF665x_Boot_state;
				break;
			case 1:
				_debug("status: idle", ret);
				*status = eDevTEF665x_Idle_state;
				break;
			default:
				_debug("status: active", ret);
				*status = eDevTEF665x_Active_state;
				break;
		}
	}
	return ret;
}

static int tef665x_power_on(int i2c_file_desc)
{
	int ret;
	TEF665x_STATE status;
	usleep(5000);
	if(SET_SUCCESS == (ret = get_operation_status(i2c_file_desc, &status)))   //[ w 40 80 01 [ r 0000 ]
	{
		_debug("Powered ON", ret);
	}
	else
	{
		_debug("Powered ON FAILED!", ret);
	}

	return ret;
}

static int tef665x_writeTab(int i2c_file_desc,const uint8_t *tab)
{
	int ret;
	ret = write(i2c_file_desc, tab + 1, tab[0]);
	return (ret != tab[0]) ? 0 : 1;
}

static int tef665x_patch_load(int i2c_file_desc, const uint8_t *bytes, uint16_t size)
{
	uint8_t buf[25]; //the size which we break the data into, is 24 bytes.
	int ret, i;

    uint16_t num = size / 24;
	uint16_t rem = size % 24;

    buf[0] = 0x1b;

    usleep(10000);

    for(i = 0; i < num; i++)
    {
		memcpy(buf + 1, bytes + (24 * i), 24);

		ret = write(i2c_file_desc, buf, 25);

		if(ret != 25)
		{
			_debug("FAILED, send patch error! in pack no", i);
			return false;
		}
		usleep(50);
	}

    memcpy(buf + 1, bytes + (num * 24), rem);

    ret = write(i2c_file_desc, buf, rem);
		if(ret != rem)
		{
			_debug("FAILED, send patch error at the end!", 0);
			return false;
		}
	usleep(50);

	_debug("return value", 1);
	return true;
}

static int tef665x_patch_init(int i2c_file_desc)
{
	int ret = 0;
	ret = tef665x_writeTab(i2c_file_desc, tef665x_patch_cmdTab1);  //[ w 1C 0000 ]
	if(!ret)
	{
		_debug("1- tab1 load FAILED", ret);
		return ret;
	}

	ret = tef665x_writeTab(i2c_file_desc, tef665x_patch_cmdTab2);  //[ w 1C 0074 ]
	if(!ret)
	{
		_debug("2- tab2 load FAILED", ret);
		return ret;
	}

	ret = tef665x_patch_load(i2c_file_desc, pPatchBytes, patchSize); //table1
	if(!ret)
	{
		_debug("3- pPatchBytes load FAILED", ret);
		return ret;
	}

	ret = tef665x_writeTab(i2c_file_desc, tef665x_patch_cmdTab1); //[ w 1C 0000 ]
	if(!ret)
	{
		_debug("4- tab1 load FAILED", ret);
		return ret;
	}

	ret = tef665x_writeTab(i2c_file_desc, tef665x_patch_cmdTab3); //[ w 1C 0075 ]
	if(!ret)
	{
		_debug("5- tab3 load FAILED", ret);
		return ret;
	}

	ret = tef665x_patch_load(i2c_file_desc, pLutBytes, lutSize); //table2
	if(!ret)
	{
		_debug("6- pLutBytes load FAILED", ret);
		return ret;
	}

	ret = tef665x_writeTab(i2c_file_desc, tef665x_patch_cmdTab1); //[ w 1C 0000 ]
	if(!ret)
	{
		_debug("7- tab1 load FAILED", ret);
		return ret;
	}
	_debug("patch loaded", ret);
	return ret;
}

//Command start will bring the device into? idle state�: [ w 14 0001 ]
static int tef665x_start_cmd(int i2c_file_desc)
{

	int ret;
	unsigned char  buf[3];

	buf[0] = 0x14;
	buf[1] = 0;
	buf[2] = 1;

	ret = write(i2c_file_desc, buf, 3);

	if (ret != 3)
	{
		_debug("start cmd FAILED", 0);
		return 0;
	}
	_debug("return true", 1);
	return 1;
}

static int tef665x_boot_state(int i2c_file_desc)
{
	int ret=0;
	if(1 == tef665x_patch_init(i2c_file_desc))
	{
		_debug("return true", 1);
	}
	else
	{
		_debug("return value", 0);
		return 0;
	}

	usleep(50000);

	if(1 == tef665x_start_cmd(i2c_file_desc))
	{
		_debug("'start cmd'return true", 1);
	}
	else
	{
		_debug("return value", 0);
		return 0;
	}

	usleep(50000);

	return ret;
}

/*
module 64 APPL
cmd 4 Set_ReferenceClock frequency

index
1 frequency_high
	[ 15:0 ]
	MSB part of the reference clock frequency
	[ 31:16 ]
2 frequency_low
	[ 15:0 ]
	LSB part of the reference clock frequency
	[ 15:0 ]
	frequency [*1 Hz] (default = 9216000)
3 type
	[ 15:0 ]
	clock type
	0 = crystal oscillator operation (default)
	1 = external clock input operation
*/
static int tef665x_appl_set_referenceClock(uint32_t i2c_file_desc, uint16_t frequency_high, uint16_t frequency_low, uint16_t type)
{
	return tef665x_set_cmd(i2c_file_desc, TEF665X_MODULE_APPL,
			TEF665X_Cmd_Set_ReferenceClock,
			9,
			frequency_high, frequency_low, type);
}

static int appl_set_referenceClock(uint32_t i2c_file_desc, uint32_t frequency, bool is_ext_clk)  //0x3d 0x900
{
	return tef665x_appl_set_referenceClock(i2c_file_desc,(uint16_t)(frequency >> 16), (uint16_t)frequency, is_ext_clk);
}

/*
module 64 APPL
cmd 5 Activate mode

index
1 mode
	[ 15:0 ]
	1 = goto �active� state with operation mode of �radio standby�
*/
static int tef665x_appl_activate(uint32_t i2c_file_desc ,uint16_t mode)
{
	return tef665x_set_cmd(i2c_file_desc, TEF665X_MODULE_APPL,
			TEF665X_Cmd_Activate,
			5,
			mode);
}

static int appl_activate(uint32_t i2c_file_desc)
{
	return tef665x_appl_activate(i2c_file_desc, 1);
}
/*
module 48 AUDIO
cmd 22 set_dig_io signal, format, operation, samplerate

index
1 signal
[ 15:0 ]
	digital audio input / output
	32 = I²S digital audio IIS_SD_0 (input)
	33 = I²S digital audio IIS_SD_1 (output)
(2) mode
	0 = off (default)
	1 = input (only available for signal = 32)
	2 = output (only available for signal = 33)
(3) format
	[ 15:0 ]
	digital audio format select
	16 = I²S 16 bits (fIIS_BCK = 32 * samplerate)
	32 = I²S 32 bits (fIIS_BCK = 64 * samplerate) (default)
	272 = lsb aligned 16 bit (fIIS_BCK = 64 * samplerate)
	274 = lsb aligned 18 bit (fIIS_BCK = 64 * samplerate)
	276 = lsb aligned 20 bit (fIIS_BCK = 64 * samplerate)
	280 = lsb aligned 24 bit (fIIS_BCK = 64 * samplerate)
(4) operation
	[ 15:0 ]
	operation mode
	0 = slave mode; IIS_BCK and IIS_WS input defined by source (default)
	256 = master mode; IIS_BCK and IIS_WS output defined by device
(5) samplerate
	[ 15:0 ] 3200 = 32.0 kHz
	4410 = 44.1 kHz (default)
	4800 = 48.0 kHz
*/
static int tef665x_audio_set_dig_io(uint8_t i2c_file_desc, uint16_t signal, uint16_t mode, uint16_t format, uint16_t operation, uint16_t samplerate)
{
	int ret = tef665x_set_cmd(i2c_file_desc, TEF665X_MODULE_AUDIO,
				TEF665X_Cmd_Set_Dig_IO,
				13,
				signal, mode, format, operation, samplerate);
	if(ret)
	{
		_debug("Digital In/Out is set ", signal);
	}
	else
	{
		_debug("FAILED, return", 0);
		return 0;
	}
	return 1;
}

/*
module 32 / 33 FM / AM
cmd 85 Set_Specials ana_out, dig_out

index
1 signal
	[ 15:0 ]
	analog audio output
	128 = DAC L/R output
2 mode
	[ 15:0 ]
	output mode
	0 = off (power down)
	1 = output enabled (default)
*/

static int tef665x_audio_set_ana_out(uint32_t i2c_file_desc, uint16_t signal,uint16_t mode)
{
	int ret = tef665x_set_cmd(i2c_file_desc, TEF665X_MODULE_AUDIO,
		      TEF665X_Cmd_Set_Ana_Out,
			  7,
			  signal, mode);
	if(ret)
	{
		_debug("analog output is set to ", mode);
	}
	else
	{
		_debug("FAILED, return", 0);
		return 0;
	}
	return 1;

}

/*
module 48 AUDIO
cmd 13 Set_Output_Source

index
1 signal
	[ 15:0 ]
	audio output
	33 = I2S Digital audio
	128 = DAC L/R output (default)
2 source
	[ 15:0 ]
	source
	4 = analog radio
	32 = i2s digital audio input
	224 = audio processor (default)
	240 = sin wave generator
*/
static int  tef665x_set_output_src(uint32_t i2c_file_desc, uint8_t signal, uint8_t src)
{
	int ret = tef665x_set_cmd(i2c_file_desc, TEF665X_MODULE_AUDIO,
			  TEF665X_Cmd_Set_Output_Source,
			  7,
			  signal, src);
	if(ret)
	{
		_debug("Output is set ", signal);
	}
	else
	{
		_debug("FAILED, return", 0);
		return 0;
	}
	return 1;
}

static int tef665x_idle_state(int i2c_file_desc)
{
	TEF665x_STATE status;

	//mdelay(50);

	if(SET_SUCCESS == get_operation_status(i2c_file_desc, &status))
	{
		_debug("got operation status", 1);
 	    if(status != eDevTEF665x_Boot_state)
		{
			_debug("not in boot status", 1);

			if(SET_SUCCESS == appl_set_referenceClock(i2c_file_desc, TEF665x_REF_CLK, TEF665x_IS_CRYSTAL_CLK)) //TEF665x_IS_EXT_CLK
			{
				_debug("set the clock", TEF665x_REF_CLK);
				if(SET_SUCCESS == appl_activate(i2c_file_desc))// APPL_Activate mode = 1.[ w 40 05 01 0001 ]
				{
					//usleep(100000); //Wait 100 ms
					_debug("activate succeed", 1);
					return 1;
				}
				else
				{
					_debug("activate FAILED", 1);
				}
			}
			else
			{
				_debug("set the clock FAILED", TEF665x_REF_CLK);
			}

		}
		else
		{
			_debug("did not get operation status", 0);
		}

	}
	_debug("return value", 0);
	return 0;
}

static int tef665x_para_load(uint32_t i2c_file_desc)
{
	int i;
	int r;
	const uint8_t *p = init_para;

	for(i = 0; i < sizeof(init_para); i += (p[i]+1))
	{
		if(SET_SUCCESS != (r = tef665x_writeTab(i2c_file_desc, p + i)))
		{
			break;
		}
	}

	//Initiate RDS
	tef665x_set_rds(i2c_file_desc);

	_debug("return value", r);
	return r;
}

/*
module 32 / 33 FM / AM
cmd 1 Tune_To mode, frequency

index
1 mode
	[ 15:0 ]
	tuning actions
	0 = no action (radio mode does not change as function of module band)
	1 = Preset Tune to new program with short mute time
	2 = Search Tune to new program and stay muted
	FM 3 = AF-Update Tune to alternative frequency, store quality
	and tune back with inaudible mute
	4 = Jump Tune to alternative frequency with short
	inaudible mute
	5 = Check Tune to alternative frequency and stay
	muted
	AM 3 � 5 = reserved
	6 = reserved
	7 = End Release the mute of a Search or Check action
	(frequency is not required and ignored)
2 frequency
[ 15:0 ]
	tuning frequency
	FM 6500 � 10800 65.00 � 108.00 MHz / 10 kHz step size
	AM LW 144 � 288 144 � 288 kHz / 1 kHz step size
	MW 522 � 1710 522 � 1710 kHz / 1 kHz step size
	SW 2300 � 27000 2.3 � 27 MHz / 1 kHz step size
*/
static int tef665x_radio_tune_to (uint32_t i2c_file_desc, bool fm, uint16_t mode,uint16_t frequency )
{
	return tef665x_set_cmd(i2c_file_desc, fm ? TEF665X_MODULE_FM: TEF665X_MODULE_AM,
			TEF665X_Cmd_Tune_To,
			( mode <= 5 ) ? 7 : 5,
			mode, frequency);
}

static int FM_tune_to(uint32_t i2c_file_desc, AR_TuningAction_t mode, uint16_t frequency)
{
	int ret = tef665x_radio_tune_to(i2c_file_desc, 1, (uint16_t)mode, frequency);
	_debug("return value", ret);
	return ret;
}

static int AM_tune_to(uint32_t i2c_file_desc, AR_TuningAction_t mode,uint16_t frequency)
{
	int ret = tef665x_radio_tune_to(i2c_file_desc, 0, (uint16_t)mode, frequency);
	_debug("return value", ret);
	return ret;
}

/*
module 48 AUDIO
cmd 11 Set_Mute mode

index
1 mode
	[ 15:0 ]
	audio mute
	0 = mute disabled
	1 = mute active (default)
*/
int tef665x_audio_set_mute(uint32_t i2c_file_desc, uint16_t mode)
{
	int ret = tef665x_set_cmd(i2c_file_desc, TEF665X_MODULE_AUDIO,
			  TEF665X_Cmd_Set_Mute,
			  5,
			  mode);
	if(ret)
	{
		_debug("mute state changed , mode", mode);
	}
	else
	{
		_debug("FAILED, return", 0);
		return 0;
	}
	return 1;
}

/*
module 48 AUDIO
cmd 10 Set_Volume volume

index
1 volume
	[ 15:0 ] (signed)
	audio volume
	-599 � +240 = -60 � +24 dB volume
	0 = 0 dB (default)f665x_patch_init function:  "3"t,int16_t volume)
*/
static int tef665x_audio_set_volume(uint32_t i2c_file_desc, uint16_t volume)
{
	return tef665x_set_cmd(i2c_file_desc, TEF665X_MODULE_AUDIO,
			TEF665X_Cmd_Set_Volume,
			5,
			volume*10);
}
/*
module 64 APPL
cmd 130 Get_Identification
index
1 device
2 hw_version
3 sw_version
*/
int appl_get_identification(int i2c_file_desc)
{
    uint8_t buf[6];
    int ret;

    ret = tef665x_get_cmd(i2c_file_desc, TEF665X_MODULE_APPL,
            TEF665X_Cmd_Get_Identification,
			buf, sizeof(buf));
// should be completed for further use
// extracting chip versions ...
    if(ret == SET_SUCCESS)
	{
		for(int i = 0; i<6;i++)
			printf("buf[%i] = %x\n", i, buf[i]);
		return 1;
	}
	_debug("return value", 0);
	return 0;
}


//mute=1, unmute=0
int audio_set_mute(uint32_t i2c_file_desc, bool mute)
{
	return tef665x_audio_set_mute(i2c_file_desc, mute);//AUDIO_Set_Mute mode = 0 : disable mute
}

//-60 � +24 dB volume
int audio_set_volume(uint32_t i2c_file_desc, int vol)
{
	return tef665x_audio_set_volume(i2c_file_desc, (uint16_t)vol);
}

/*
module 64 APPL
cmd 1 Set_OperationMode mode

index
1 mode
	[ 15:0 ]
	device operation mode
	0 = normal operation
	1 = radio standby mode (low-power mode without radio functionality)
	(default)
*/

static int tef665x_audio_set_operationMode(uint32_t i2c_file_desc, uint16_t mode)
{
	_debug("normal: 0   standby: 1   requested", 1);
	int ret = tef665x_set_cmd(i2c_file_desc, TEF665X_MODULE_APPL,
			  TEF665X_Cmd_Set_OperationMode,
			  5,
			  mode);
	if(ret)
	{
		_debug("was able to set the mode", ret);
	}
	else
	{
		_debug("FAILED, return", 0);
		return 0;
	}
	return 1;
}



//TRUE = ON;
//FALSE = OFF
static void radio_powerSwitch(uint32_t i2c_file_desc, bool OnOff)
{
	tef665x_audio_set_operationMode(i2c_file_desc, OnOff? 0 : 1);//standby mode = 1
}

static void radio_modeSwitch(uint32_t i2c_file_desc, bool mode_switch, AR_TuningAction_t mode, uint16_t frequency)
{

	if(mode_switch)	//FM
	{
		FM_tune_to(i2c_file_desc, mode, frequency);
	}
	else //AM
	{
		AM_tune_to(i2c_file_desc, mode, frequency);
	}
}

/*
module 32 FM
cmd 81 Set_RDS

index
1 mode
	[ 15:0 ]
	RDS operation mode
	0 = OFF
	1 = decoder mode (default), output of RDS groupe data (Block A, B, C, D)
        from get_rds_status, get_rds_data FM cmd 130/131

2 restart
    [ 15:0 ]
    RDS decoder restart
    0 = no control
    1 = manual restart, starlooking for new RDS data immidiately
    2 = automatic restart after tuning (default)
    3 = flush, empty RDS output buffer.

3 interface
    [ 15:0 ]
    0 = no pin interface.
    2 = data available status output; active low (GPIO feature 'DAVN')
    4 = legecy 2-wire demodulator data and clock output ('RDDA' and 'RDCL')
*/
int tef665x_set_rds(uint32_t i2c_file_desc)
{
    return tef665x_set_cmd(i2c_file_desc, TEF665X_MODULE_FM,
            TEF665X_Cmd_Set_RDS,
            9,//Total Bytes to be sent
            TEF665X_Cmd_Set_RDS_mode, // default
        	TEF665X_Cmd_Set_RDS_autorestart, // restart after tune
	        0x002 // no interface
            );
}


/*
 * @brief Adding Alternative Frequencies to RDS Data Structure
 * 
 * @param uint8_t* : raw data of alternative frequency (Group 0A of RDS)
 * @param rds_data_t* : Pointer to RDS Data Structure
 * @return void
 */
void Extract_Alt_Freqs(uint8_t* buf,rds_data_t *Rds_STU)
{	
	for(int Buffer_Index=6;Buffer_Index<8;Buffer_Index++)
	{
		if(buf[Buffer_Index]>204){
			if(250>buf[Buffer_Index]&&buf[Buffer_Index]>224)
			{
				Rds_STU->Num_AlterFreq=buf[Buffer_Index]-224;

				if(Rds_STU->Alternative_Freq_Counter == Rds_STU->Num_AlterFreq)
				{
					Rds_STU->Alternative_Freq_Counter = 0;
				}
				AlterFreqOffset=87500000;
			}
			else if(buf[Buffer_Index]==205)
			{
				fprintf(stderr, "Filler Code");
			}
			else if(buf[Buffer_Index]==224)
			{
				fprintf(stderr, "No AF Exists");
			}
			else if(buf[Buffer_Index]==250)
			{
				fprintf(stderr, "An LF/MF Frequency Follows");
				AlterFreqOffset=144000;
			}
			else if(buf[Buffer_Index]>250)
			{
				printf("Alternative Frequency Not Assigned");
			}
		}
		else if(buf[Buffer_Index]>0)
		{
			if(AlterFreqOffset == 87500000)
			{	
				Rds_STU->Alternative_Freq[Rds_STU->Alternative_Freq_Counter]=
					buf[Buffer_Index] * 100000 + AlterFreqOffset;

				Rds_STU->Alternative_Freq_Counter++;

				if(Rds_STU->Alternative_Freq_Counter == Rds_STU->Num_AlterFreq)
				{
					Rds_STU->Alternative_Freq_Counter = 0;
				}
			}
			else if(AlterFreqOffset == 144000)
			{
				Rds_STU->Alternative_Freq[Rds_STU->Alternative_Freq_Counter]=
					((uint32_t)buf[Buffer_Index]) * 9000 + AlterFreqOffset;

				Rds_STU->Alternative_Freq_Counter++;

				if(Rds_STU->Alternative_Freq_Counter == Rds_STU->Num_AlterFreq)
				{
					Rds_STU->Alternative_Freq_Counter = 0;
				}
			}
			else
			{
				printf("Alternative Frequency is not defined");
			}
		}
		else
		{
			fprintf(stderr, "Alternative Frequency- Not to be used");
		}		
	}
}

/*
 * @brief Checking rds error code (determined by decoder)
 * 	
 *  0 : no error; block data was received with matching data and syndrome
 *  1 : small error; possible 1 bit reception error detected; data is corrected
 *  2 : large error; theoretical correctable error detected; data is corrected
 *  3 : uncorrectable error; no data correction possible
 * 
 * @param Errors : Error Code of blocks A,B,C and D of RDS
 * @return void
 */
void Check_RDS_Error(uint8_t Errors[])
{
	for (int i=0;i<4;i++){
		if(Errors[i]==1){
			printf("RDS Block %d Reception Error; small error; possible 1 bit reception error detected; data is corrected",i+1);
		}
		else if(Errors[i]==2){
			printf("RDS Block %d Reception Error; large error; theoretical correctable error detected; data is corrected",i+1);
		}
		else if(Errors[i]==3){
			fprintf(stderr, "RDS Block %d Reception Error; uncorrectable error; no data correction possible",i+1);
		}
	}
}

/*
 * @brief Getting rds_data_t and Process its raw_data
 * 	
 * @param rds_data_t * : Pointer to latest RDS Data Structure
 */
void *Process_RDS_Words(void* rds_words){
	pthread_detach(pthread_self());
	
	rds_data_t *Rds_STU  = rds_words;
	uint8_t    *raw_data = Rds_STU->raw_data;
	int8_t 	   group_Ver = -1;
	uint8_t    GType0	 = 0;
	bool       DI_Seg    = 0;
	bool       M_S       = 0;
	bool       TA        = 0;

	//Parse 1st Section
	bool    DataAvailable = (raw_data[0] >> 7) & 1;
	bool    DataLoss      = (raw_data[0] >> 6) & 1 == 1;
	bool    DataAvailType = (raw_data[0] >> 5) & 1 == 0;
	bool    GroupType     = (raw_data[0] >> 4) & 1;
	bool    SyncStatus    = (raw_data[0] >> 1) & 1;

	//Parse Last Section(Error Codes)
	uint8_t Error_A = raw_data[10] >> 6;
	uint8_t Error_B = raw_data[10] >> 4 & 3;
	uint8_t Error_C = raw_data[10] >> 2 & 3;
	uint8_t Error_D = raw_data[10] & 3;
	uint8_t Errors[]={Error_A,Error_B,Error_C,Error_D};

	//Inform user about Error Blocks Status Codes
	Check_RDS_Error(Errors);

	if(Error_A==0){
		//Bytes 2 and 3 are inside Block A
		//raw_data[2]and raw_data[3] Contains PI Code
		Rds_STU->PICode=Convert8bto16b(&raw_data[2]);
	}
	else{
		fprintf(stderr, "Error_A=%d",Error_A);
	}

	bool GTypeVer=GType0;
	uint16_t GType=raw_data[4]>>4;
	//Bytes 4 and 5 are inside Block B
	if(Error_B==0){
		GTypeVer=raw_data[4]>>3 & 1;
		GType=raw_data[4]>>4;
		Rds_STU->TrafficProgram=raw_data[4]>>2&1;
		Rds_STU->PTY_Code= (raw_data[4] & 3) << 3 | raw_data[5] >> 5;
	}

	//Position Of Character
	uint8_t CharPos=0;

	//Extract Data based on Group Type values
	switch (GType)
	{
	case 0:
	{
		if(Error_B==0)
		{
			CharPos = raw_data[5] & 3;

			Rds_STU->TrafficAnnouncement = raw_data[5]  >> 4 & 1;
			Rds_STU->Music_Speech        = raw_data[5]  >> 3 & 1;
			Rds_STU->DI_Seg		         = (raw_data[5] >> 2 & 1) * (2 ^ (3 - CharPos));
		}

		if(Error_C==0)
		{
			//Group Type 0A
			if (GType==0)
			{
				Extract_Alt_Freqs(raw_data,Rds_STU);
			}

			//Group Type 0B
			else
			{
				Rds_STU->PICode=Convert8bto16b(&raw_data[6]);
			}
		}

		if(Error_D == 0 && Error_B == 0)
		{
			if(raw_data[8] != 0x7f)
			{
				Rds_STU->PS_Name[CharPos*2]   = raw_data[8];
			}
			else
			{
				Rds_STU->PS_Name[CharPos*2]   =  (char)'\0';
			}
			if(raw_data[9] != 0x7f)
			{
				Rds_STU->PS_Name[CharPos*2+1] = raw_data[9];
			}
			else
			{
				Rds_STU->PS_Name[CharPos*2+1] =  (char)'\0';
			}
		}
	}
	break;
	case 1:
	{
		//Group Type 1A
		if(GTypeVer == 0)
		{
			if(Error_D == 0)
			{
				Rds_STU->Day  = raw_data[8] >> 3;
				Rds_STU->Hour = raw_data[8] >> 3;
				Rds_STU->Min  = ((raw_data[8] & 7) << 2) | (raw_data[9] >> 6) ;
			}
		}
	}
	break;
	case 2:
	{
		//Group Type 2A:
		if(GTypeVer == 0)
		{
			uint8_t Text_pos = raw_data[5] & 15;
			
			if(Error_B == 0 && Error_C == 0)
			{
				
				if(raw_data[6] !=0x7f && raw_data[6] != '\n')
				{
					Rds_STU->RT[Text_pos*4] = raw_data[6];
				}
				else{
					Rds_STU->RT[Text_pos*4] = (char)'\0';
				}
				if(raw_data[7]!=0x7f&&raw_data[7]!='\n')
				{
					Rds_STU->RT[Text_pos*4+1] = raw_data[7];
				}
				else
				{
					Rds_STU->RT[Text_pos*4+1] =  (char)'\0';
				}
			}
			if(Error_B == 0 && Error_D == 0)
			{
				if(raw_data[8] != 0x7f && raw_data[8] != '\n')
				{
					Rds_STU->RT[Text_pos*4+2] = raw_data[8];
				}
				else{
					Rds_STU->RT[Text_pos*4+2] = (char)'\0';
				}
				if(raw_data[9] != 0x7f && raw_data[9] != '\n')
				{
					Rds_STU->RT[Text_pos*4+3] = raw_data[9];
				}
				else
				{
					Rds_STU->RT[Text_pos*4+3] = (char)'\0';
				}
			}
		}
		
		//Group Type 2B:
		else{
			if(Error_B==0 && Error_D==0)
			{
				//Clear All Radio Text if flag was changed
				if(raw_data[5] >> 4 & 1 != Rds_STU->Text_Changed)
				{
					memcpy(Rds_STU->RT, _Temp , 64);
				}

				uint8_t Text_pos = raw_data[5] & 15;
				if(raw_data[8] != 0x7f && raw_data[8] != '\n')
				{

					Rds_STU->RT[Text_pos*2] = raw_data[8];
				}
				else{
					Rds_STU->RT[Text_pos*2] =  (char)'\0';
				}
				if(raw_data[9] != 0x7f && raw_data[9] != '\n')
				{
					Rds_STU->RT[Text_pos*2+1] = raw_data[9];
				}
				else
				{
					Rds_STU->RT[Text_pos*2+1] =  (char)'\0';
				}
			}
		}
	}
	break;
	case 4:
	{
		//Group Type 4A
		if(GTypeVer == 0)
		{
			if(Error_B == 0 && Error_C == 0 && Error_D == 0)
			{
				//Following caclulations are based on RDS Standard
				uint32_t  Modified_Julian_Day = ((raw_data[5] & 3) << 15) | (raw_data[6] << 7) | (raw_data[7]>>1);
				int y2 =  (int)((((double)Modified_Julian_Day)-15078.2)/365.25);
				int m2 =  (int)((((double)Modified_Julian_Day)-14956.1-((double)y2*365.25))/30.6001);
				int d2 =  (double)Modified_Julian_Day-14956-(int)(y2*365.25)-(int)(m2*30.6001);
				int k  =  0;

				if(m2  == 14 || m2 == 15)
				{
					k  = 1;
				}

				Rds_STU->Day      = d2;
				Rds_STU->Month    = m2 - 1 + k * 12;
				Rds_STU->Year     = y2 + k;

				uint8_t UTCHour   = ((raw_data[7] & 1)  << 4) | (raw_data[8] >> 4);
				uint8_t UTCMinute = ((raw_data[8] & 15) << 2) | (raw_data[9] >> 6);

				//Check Negative Offset
				bool    NegOff     = raw_data[9] & 32;
				uint8_t LocTimeOff = raw_data[9] & 31;

				if(!NegOff)
				{
					Rds_STU->Min = UTCMinute + LocTimeOff % 2;
					while(UTCMinute > 60)
					{
						UTCHour++;
						UTCMinute = UTCMinute - 60;
					}

					Rds_STU->Hour = UTCHour + LocTimeOff / 2;
					while(Rds_STU->Hour > 24){
						Rds_STU->Hour = Rds_STU->Hour - 24;
					}
					
					
				}
				else{
					Rds_STU->Min = UTCMinute + LocTimeOff % 2;
					while(UTCMinute < 0)
					{
						UTCHour--;
						UTCMinute = UTCMinute + 60;
					}
					Rds_STU->Hour = UTCHour + LocTimeOff / 2;
					while(Rds_STU->Hour<0)
					{
						Rds_STU->Hour = Rds_STU->Hour + 24;
						
					}
				}
			}
		}
		//Group Type 4B
		else
		{
			printf("Groupe Type 4B are not supported yet");
		}
	}
	case 8:
	{
		printf("Groupe Type 8A and 8B are not supported yet");
		
	}
	case 10:
	{
		printf("Groupe Type 10A and 10B are not supported yet");
		/*
		if(Error_B == 0){
			uint8_t pos = 0;
			pos=(raw_data[5] & 1) * 4;

			if(Error_C == 0){
				Rds_STU->PTYN[pos]   = raw_data[6];
				Rds_STU->PTYN[pos+1] = raw_data[7];
				Rds_STU->PTYN_Size   = pos + 2;
			}
			if(Error_D == 0){
				Rds_STU->PTYN[pos+2] = raw_data[8];
				Rds_STU->PTYN[pos+3] = raw_data[9];
				Rds_STU->PTYN_Size   = pos + 4;
			}
		}
		/**/
	}
	break;
	default:
		fprintf(stderr, "Unsupported Group %d",GType);
		break;
	}

	if(!DataAvailable)
	{
		fprintf(stderr, "RDS Data is not available");
	}

	if(DataLoss)
	{
		fprintf(stderr, "previous data was not read, replaced by newer data");
	}

	if(GroupType == 0)
	{
		group_Ver = 0;
	}
	else
	{
		group_Ver = 1;
	}

	if(!SyncStatus)
	{
		fprintf(stderr, " RDS decoder not synchronized; no RDS data found");
	}

	if(GroupType != GTypeVer)
	{
		fprintf(stderr, "Version is not Correct?");
	}
}

/*
module 32 FM
cmd 131 get RDS data

index
1 status
    [ 15:0 ]
    FM RDS reception status.
    [15] = dta availableflag
        0 = no data
        1 = data available
    [14] = data loss flag
        0 = no data loss
        1 = previose data not read, replaced by newer data.
    [13] = data available type
        0 = group data; continuos operation.
        1 = first PI data;data with PI code following decoder sync.
    [12] = groupe type.
        0 = type A; A-B-C-D group (with PI code in the block A)
        1 = type B; A-B-C'-D group (with PI code in the block A and C')
    [ 8:0 ] reserved

2 A_Block
    [ 15:0 ] = A block data

3 B_Block
    [ 15:0 ] = B block data

4 C_Block
    [ 15:0 ] = C block data

5 D_Block
    [ 15:0 ] = D block data

6 dec error
    [ 15:0 ]
    error code determined by decoder
    [ 15:14 ] = A block error
    [ 13:12 ] = B block error
    [ 11:10 ] = C block error
    [ 9:8 ] = D block error
    0 = no error found
    1 = small error, correctable. data is corrected.
    2 = larg error, correctable. data is corrected.
    3 = uncorrectable error.
    [ 7:0 ] = reserved.
*/
/*
 * @brief Get RDS Data fron Tef-665
 * 	
 * Getting RDS Data From I2C and Calling a thread to process raw data
 * 
 * @param i2c_file_desc : I2C File Descriptor
 * @param Rds_STU : RDS Data Structure
 * 
 */
int tef665x_get_rds_data(uint32_t i2c_file_desc, rds_data_t *Rds_STU)
{

    int     ret;
    uint8_t buf[12];
	
    ret = tef665x_get_cmd(i2c_file_desc, TEF665X_MODULE_FM,
            TEF665X_Cmd_Get_RDS_Data,
            buf, sizeof(buf));

    if(ret == 1) {
		memcpy(Rds_STU->raw_data,buf,12);
		pthread_t t0;
		pthread_create(&t0, NULL,Process_RDS_Words ,(void *) (Rds_STU));
    }
    return ret;
}

void Clear_RDS_Data(rds_data_t *Rds_STU){
	
	Rds_STU-> Text_Changed=0;
	Rds_STU-> TrafficAnnouncement=0;
	Rds_STU-> TrafficProgram=0;
	Rds_STU-> Music_Speech=0;
	
	Rds_STU-> DI_Seg=0;
	Rds_STU-> PTY_Code=0;
	Rds_STU-> Num_AlterFreq=0;
	Rds_STU->PTYN_Size=0;
	
	Rds_STU-> Day=0;
	Rds_STU-> Month=0;
	Rds_STU-> Year=0;

	Rds_STU-> Hour=0;
	Rds_STU-> Min=0;

	/*memcpy(Rds_STU->Alternative_Freq,_Temp,25);/**/
	for(uint8_t i=0;i<25;i++){
		Rds_STU->Alternative_Freq[i]=0;
	}
	memcpy(Rds_STU-> PS_Name,_Temp,8);
	Rds_STU-> PS_Name[0]='\0';
	memcpy(Rds_STU-> RT,_Temp,64);
	Rds_STU-> RT[0]='\0';
	memcpy(Rds_STU-> PTYN,_Temp,8);
	Rds_STU-> PTYN[0]='\0';

	Rds_STU-> PICode=0;
	Rds_STU->Alternative_Freq_Counter=0;
	Rds_STU->PTYN_Size=0;
}

//Check if RDS is available
int tef665x_get_rds_status(uint32_t i2c_file_desc, uint16_t *status)
{
    int ret = 0;
    uint8_t buf[2];

    ret = tef665x_get_cmd(i2c_file_desc, TEF665X_MODULE_FM,
            TEF665X_Cmd_Get_RDS_Status,
            buf, sizeof(buf));

    if(ret == 1){
        status[0] =buf[0];
		status[1] =buf[1];
    }

    return ret;
}

static int tef665x_wait_active(uint32_t i2c_file_desc)
{
	TEF665x_STATE status;
	//usleep(50000);
	if(SET_SUCCESS == appl_get_operation_status(i2c_file_desc, &status))
	{
		printf("got status", 1);
		if((status != eDevTEF665x_Boot_state) && (status != eDevTEF665x_Idle_state))
		{
			printf("active status", 1);

			if(SET_SUCCESS == tef665x_para_load(i2c_file_desc))
			{
				_debug("parameters loaded", 1);
			}
			else
			{
				_debug("parameters not loaded", 0);
				return 0;
			}

			if(current_band == RADIO_BAND_FM){
				FM_tune_to(i2c_file_desc, eAR_TuningAction_Preset, current_fm_frequency / 10000);// tune to min
			} else {
				AM_tune_to(i2c_file_desc, eAR_TuningAction_Preset, current_am_frequency / 1000);// tune to min
			}

			if(SET_SUCCESS == audio_set_mute(i2c_file_desc, 1))//unmute=0
			{
				_debug("muted", 1);
			}
			else
			{
				_debug("not muted", 0);
				return 0;
			}

			// //if(SET_SUCCESS == audio_set_volume(i2c_file_desc, 35))//set to -10db
			// {
			// 	_debug("set vol to", 25);
			// }
			
			// else
			// {
			// 	_debug("vol not set", 0);
			// 	return 0;
			// }
			return 1;
		}
	}

	return 0;
}

static void tef665x_chip_init(int i2c_file_desc)
{
	if(1 == tef665x_power_on(i2c_file_desc)) _debug("tef665x_power_on", 1);
	usleep(50000);
	if(1 == tef665x_boot_state(i2c_file_desc)) _debug("tef665x_boot_state", 1);
	usleep(100000);
	if(1 == tef665x_idle_state(i2c_file_desc)) _debug("tef665x_idle_state", 1);
	usleep(200000);
	if(1 == tef665x_wait_active(i2c_file_desc)) _debug("tef665x_wait_active", 1);
	//if you want to use analog output comment below command, or pass 1 to it.
	if(SET_SUCCESS != tef665x_audio_set_ana_out(i2c_file_desc, TEF665X_Cmd_Set_Output_signal_dac, 0))
	{
		_debug("Set DAC to OFF failed", 0);
		//return 0;
	}

	if(SET_SUCCESS != tef665x_set_output_src(i2c_file_desc, TEF665X_Cmd_Set_Output_signal_i2s,
															TEF665X_Cmd_Set_Output_source_aProcessor))
	{
		_debug("Set output failed", 0);
		//return 0;
	}
	//this is needed to use digital output
	if(SET_SUCCESS != tef665x_audio_set_dig_io(i2c_file_desc, TEF665X_AUDIO_CMD_22_SIGNAL_i2s1,
																TEF665X_AUDIO_CMD_22_MODE_voltage,
																TEF665X_AUDIO_CMD_22_FORMAT_16,
																TEF665X_AUDIO_CMD_22_OPERATION_slave,
																TEF665X_AUDIO_CMD_22_SAMPLERATE_48K))
	{
		_debug("Setup i2s failed", 0);
		//return 0;
	}


}


static int i2c_init(const char *i2c, int state, uint32_t *i2c_file_desc)
{
    int fd = 0, t;

	if(state == _open)
	{
		fd = open(i2c, O_RDWR);

		if(fd < 0)
		{
			_debug("could not open %s", i2c);
			return fd;
		}

		t = ioctl(fd, I2C_SLAVE, I2C_ADDRESS);
		if (t < 0)
		{
			_debug("could not set up slave ", 0);
			return t;
		}
		*i2c_file_desc = fd;
	}
	else
	{
		close(*i2c_file_desc);
	}

	return 0;
}

static void tef665x_start(void)
{
	int ret;

	if(!initialized)
		return;

	_debug("file_desc ", file_desc);

	audio_set_mute(file_desc, 0);

 	if(!running) {

		// Start pipeline
		ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
		_debug("gst_element_set_state to play", ret);
		running = true;
	}
 }

/*
 * @brief Send_Rds_Result to rds subscribers
 *
 * @param rds_data_t : a rds message structure
 * @return The JsonObject of rds info
 */
void *Send_Rds_Result(rds_data_t* RDS_Message){
	//Kill the thread when it was over
    pthread_detach(pthread_self());

	json_object *ret_json;
	json_object *Alternative_Freqs;
	

	ret_json = json_object_new_object();
	Alternative_Freqs=json_object_new_array();
	
	
	
	for(uint8_t af=0 ; af<25 ; af++)
	{
		if(RDS_Message->Alternative_Freq[af]!=NULL&&RDS_Message->Alternative_Freq[af]!=0)
		{
			json_object_array_add(Alternative_Freqs,json_object_new_int(RDS_Message->Alternative_Freq[af]));
		}
	}
	
	//Prepare JSon Object
	json_object_object_add(ret_json, "name"			, json_object_new_string(RDS_Message->PS_Name));
	json_object_object_add(ret_json, "radiotext"	, json_object_new_string(RDS_Message->RT));
	json_object_object_add(ret_json, "alternatives"	, (Alternative_Freqs));
	json_object_object_add(ret_json, "minute"		, json_object_new_int	(RDS_Message->Min));
	json_object_object_add(ret_json, "hour"			, json_object_new_int 	(RDS_Message->Hour));
	json_object_object_add(ret_json, "day"			, json_object_new_int 	(RDS_Message->Day));
	json_object_object_add(ret_json, "month"		, json_object_new_int	(RDS_Message->Month));
	json_object_object_add(ret_json, "year"			, json_object_new_int	(RDS_Message->Year));
	json_object_object_add(ret_json, "pi"			, json_object_new_int 	(RDS_Message->PICode));
	json_object_object_add(ret_json, "pty"			, json_object_new_int 	(RDS_Message->PTY_Code));
	json_object_object_add(ret_json, "ta"			, json_object_new_int 	(RDS_Message->TrafficAnnouncement));
	json_object_object_add(ret_json, "tp"			, json_object_new_int 	(RDS_Message->TrafficProgram));
	json_object_object_add(ret_json, "ms"			, json_object_new_int 	(RDS_Message->Music_Speech));

	//Send JsonObject to rds Subscribers
	if(rds_callback){
		rds_callback(ret_json);
	}

	return ret_json;
}

/*
 * @brief Create an infinit Loop to get RDS Packets and Send them to subscribers
 *	
 * RDS data will be available every 85 ms;
 * Currently availability of RDS is checkes by tef665x_get_rds_status function
 * 
 * @param rds_data_t : a rds message structure
 * @return The JsonObject of latest rds info
 */
void *Get_RDS_Packets(rds_data_t *StuRDS){
	pthread_detach(pthread_self());
	uint32_t fd = 0;

	int ret = i2c_init(I2C_DEV, _open, &fd);
	uint8_t status[2];
	
	ret=tef665x_get_rds_status(fd, status);

	if(ret==1){
		if(status[0]>7){
			//RDS must update all the time, except the times we are scanning or changing frequency
			//when scanning or changing frequncy, we unlock RDS_Mutex and it will end this thread
			for (int ref_cnt=0; pthread_mutex_trylock(&RDS_Mutex) != 0;ref_cnt++){
				//Get New RDS Data
				tef665x_get_rds_data(fd,StuRDS);

				//Send RDS Data after rexeiving 22 Packets
				if(ref_cnt%22==0){
					pthread_t t0;
					pthread_create(&t0, NULL,Send_Rds_Result ,(void *) (&RDS_Message));
				}

				//Wait for 85ms before reading available rds data
				usleep(85000);
			}
			pthread_mutex_unlock (&RDS_Mutex);
		}
		
		else{
			fprintf(stderr, "RDS is Not Valid0");
		}
	}

	else{
		fprintf(stderr, "RDS is Not Valid1");
	}
	i2c_init(I2C_DEV, _close, &fd);
}

/*
 * @brief Free Allocated Memory for Scan Thread and Unlock Scan Mutex
 *	
 * @param scan_data : scan_data_t contains direction of search and callback
 *                    for station_found event
 */
static void scan_cleanup_handler(void *scan_data)
{
	pthread_mutex_unlock(&scan_mutex);
	free(scan_data);
	scanning=false;
}

/*
 * @brief Create a loop to scan from current frequency to find a valid frequency
 * 
 * If found a valid frequency, send station_found to subscribers and break the loop;
 * If the direction was forward  and reach the maximum frequency, Search Must continue 
 * from minimum frequency
 * If the direction was backward and reach the minimum frequency, Search Must continue 
 * from maximum frequency
 * If no valid frequency found, scan will stop at the begining point  
 * If stop_scan called, scan_mutex will be unlocked and thread must be stopped
 * 
 * @param scan_data : scan_data_t contains direction of search and callback
 *                    for station_found event
 */
void *scan_frequencies(scan_data_t* scan_data){
	pthread_cleanup_push(scan_cleanup_handler, (void *)scan_data);

	//Kill the thread when it was over
	pthread_detach(pthread_self());
	
	//Set Scan Flag
	scanning=true;

	//"Unlock Mutex" Flag
	bool     unlck_mtx = false;
	uint32_t new_freq  = 0;
	uint32_t init_freq = 0;

	init_freq = current_band == RADIO_BAND_FM ? current_fm_frequency : current_am_frequency;

	//First Mute Current Frequency
	tef665x_search_frequency(init_freq);

	//freq_step will be negative if direction was backward and positive if direction was forward
	uint32_t freq_step = tef665x_get_frequency_step(current_band) * (scan_data->direction==RADIO_SCAN_FORWARD?1:-1);

	//Continue loop until reaching the initial frequency
	while(init_freq != new_freq)
	{
		//Check Status of scan_mutex
		unlck_mtx = pthread_mutex_trylock(&scan_mutex)==0;

		//break the loop if scan_mutex was unlocked
		if(unlck_mtx)
		{
			break;
		}

		if(current_band==RADIO_BAND_FM)
		{
			new_freq = current_fm_frequency + freq_step;

			//Searching Step is 100 KHz
			//If frequency reached to initial point, the search must stop
			while (((new_freq/10000)%10)!=0 && init_freq != new_freq){
				new_freq = new_freq+freq_step;
			}
		}
		else
		{
			new_freq = current_am_frequency + freq_step;
		}

		//Set Freq to min when it was more than Max Value
		if(new_freq>tef665x_get_max_frequency(current_band))
		{
			new_freq=tef665x_get_min_frequency(current_band);
		}

		//Set Freq to max when it was less than Min Value
		if(new_freq<tef665x_get_min_frequency(current_band))
		{
			new_freq=tef665x_get_max_frequency(current_band);
		}
		
		//Tune to new frequency
		tef665x_search_frequency(new_freq);
		
		//wait 30 ms to make sure quality data is available
		for(int i=0;i<40;i++)
		{
			usleep(1000);

			//Check scan_mutex lock for handling stop_scan
			unlck_mtx=pthread_mutex_trylock(&scan_mutex)==0;
			if(unlck_mtx)
			{
				break;
			}
		}
		if(unlck_mtx)
		{
			break;
		}

		//Get Quality of tuned frequeency
		tef665x_get_quality_info();//Get_quality_status();

		if((quality.rssi >260 /*&& ->.usn<100/**/) || quality.bandw>1200)
		{
			//Send frequency value
			if(scan_data->callback)
			{
				scan_data->callback(new_freq,NULL);
			}
			
			break;
		}
		usleep(100);
	}

	//Calling last pthread_cleanup_push
	pthread_cleanup_pop(1);
}

/*
 * @brief Get latest RDS Info and send rds jsonObject as response
 * 
 * @return: cast rds_json(json_object) to (char *) and return result as response
 */
static char *tef665x_get_rds_info(void)
{
 	//If Getting RDS Result wasn't already started, Start it now
	if(pthread_mutex_trylock(&RDS_Mutex) == 0)
	{
		//AFB_DEBUG("Create the thread.");
		pthread_create(&rds_thread, NULL,Get_RDS_Packets ,(void *) (&RDS_Message));
	}
	
	//Send latest available rds data
	json_object *rds_json=(json_object *)Send_Rds_Result(&RDS_Message);

	//Convert  json_object to char* and send it as response
	return (char *)json_object_to_json_string(rds_json);
}

/*
 * @brief Get latest quality Info and send quality parameters as response
 *
 * module 32/33 FM/AM
 * cmd 129 Get_Quality_Data
 *
 * index
 * 1 status
 *   [ 15:0 ]
 *     quality detector status
 *     [15] = AF_update flag
 * 		0 = continuous quality data with time stamp
 * 		1 = AF_Update sampled data
 * 	[14:10] = reserved
 *         0 = no data loss
 *         1 = previose data not read, replaced by newer data.
 *     [9:0] = quality time stamp
 * 		0 = tuning is in progress, no quality data available
 * 	    1 … 320 (* 0.1 ms) = 0.1 … 32 ms after tuning,
 * 			quality data available, reliability depending on time stamp
 * 		1000 = > 32 ms after tuning
 * 			quality data continuously updated
 *
 * 2 level
 * 	[ 15:0 ] (signed)
 * 	level detector result
 * 		-200 … 1200 (0.1 * dBuV) = -20 … 120 dBuV RF input level
 * 		actual range and accuracy is limited by noise and agc
 *
 * 3 usn
 *     [ 15:0 ] = noise detector
 * 		FM ultrasonic noise detector
 * 		0 … 1000 (*0.1 %) = 0 … 100% relative usn detector result
 *
 * 4 wam
 *     [ 15:0 ] = radio frequency offset
 * 		FM ‘wideband-AM’ multipath detector
 * 	0 … 1000 (*0.1 %) = 0 … 100% relative wam detector result
 *
 * 5 offset
 *     [ 15:0 ] (signed) = radio frequency offset
 * 		-1200 … 1200 (*0.1 kHz) = -120 kHz … 120 kHz radio frequency error
 * 		actual range and accuracy is limited by noise and bandwidth
 *
 * 6 bandwidth
 *     [ 15:0 ] = IF bandwidth
 * 		FM 560 … 3110 [*0.1 kHz] = IF bandwidth 56 … 311 kHz; narrow … wide
 * 		AM 30 … 80 [*0.1 kHz] = IF bandwidth 3 … 8 kHz; narrow … wide
 *
 * 7 modulation
 * 	[ 15:0 ] = modulation detector
 * 		FM 0 … 1000 [*0.1 %] = 0 … 100% modulation = 0 … 75 kHz FM dev.
 *
 * @return: cast station_quality_t pointer as response
 *
 */

static station_quality_t *tef665x_get_quality_info(void)
{
    uint32_t i2c_file_desc=0;
    uint8_t	 data[14];

    int ret = i2c_init(I2C_DEV, _open, &i2c_file_desc);
    if(current_band==RADIO_BAND_FM)
	{
		ret = tef665x_get_cmd(i2c_file_desc,  TEF665X_MODULE_FM,
		TEF665X_Cmd_Get_Quality_Data,
		data, sizeof(data));
	}
	else
	{
		ret = tef665x_get_cmd(i2c_file_desc,  TEF665X_MODULE_AM,
		TEF665X_Cmd_Get_Quality_Data,
		data, sizeof(data));
	}
	i2c_init(I2C_DEV, _close, &i2c_file_desc);

    quality.af_update  =   data[0]&0b10000000;
    quality.time_stamp = ((data[0]&0b00000011)<<8 | data[1]);
    quality.rssi	   = (data[2] << 8 | data[3] );
    quality.usn		   = (data[4] << 8 | data[5] );
    quality.bandw 	   = (data[10]<< 8 | data[11]);

    return &quality;
}

/*
 * @brief Start Scan
 *
 * @param radio_scan_direction_t direction which is the scan direction and can be
 *                               RADIO_SCAN_FORWARD or RADIO_SCAN_BACKWARD
 * @param radio_scan_callback_t  callback which is the callback for sending result of search to
 *                               station_found ecent subscribers
 * @return void
 */
static void tef665x_scan_start(radio_scan_direction_t direction,
			  radio_scan_callback_t callback,
			  void *data)
{
	//Stop RDS if enabled
	pthread_mutex_unlock (&RDS_Mutex);
	
	//Stop current scan:
	if(scanning)
	{
		tef665x_scan_stop();
	}
	
	scan_data_t *inputs;

	//Clean RDS Message since frequency will change
	Clear_RDS_Data(&RDS_Message);
	usleep(10000);

	//AFB_DEBUG("check Mutex Condition");
	
	//check if is there any activated search
	if(pthread_mutex_trylock(&scan_mutex)==0&&!scanning)
	{
		//AFB_DEBUG("Start Scanning...");
		
		inputs=malloc(sizeof(*inputs));
		if(!inputs)
			return -ENOMEM;
		
		inputs->direction= direction;
		inputs->callback= callback;
		inputs->data=data;
		
		pthread_create(&scan_thread, NULL,scan_frequencies ,(void *) inputs);
	}
}

/*
 * @brief Stop Scan
 * 
 * By unlocking scan_mutex, Scan thread will be stopped safely and update scanning flag
 * 
 * @return void
 */
static void tef665x_scan_stop(void)
{
	pthread_mutex_unlock(&scan_mutex);
	while(scanning)
	{
		usleep(100);
		//AFB_DEBUG(" Wait for unlocking scan Thread");
	}
}

/*
module 32 / 33 FM / AM
cmd 133 Get_Signal_Status | status
index
1 status
	[ 15:0 ] = Radio signal information
		[15] = 0 : mono signal
		[15] = 1 : FM stereo signal (stereo pilot detected)

		[14] = 0 : analog signal
		[14] = 1 : digital signal (blend input activated by digital processor or control)
		(TEF6659 only)
*/
radio_stereo_mode_t tef665x_get_stereo_mode(void)
{
	uint32_t i2c_file_desc = 0;
	int ret = i2c_init(I2C_DEV, _open, &i2c_file_desc);
	uint8_t data[2];
	if(current_band==RADIO_BAND_FM){
		ret = tef665x_get_cmd(i2c_file_desc,  TEF665X_MODULE_FM,
		TEF665X_Cmd_Get_Signal_Status,
		data, sizeof(data));
	}
	else{
		ret = tef665x_get_cmd(i2c_file_desc,  TEF665X_MODULE_AM,
		TEF665X_Cmd_Get_Signal_Status,
		data, sizeof(data));
	}
	i2c_init(I2C_DEV, _close, &i2c_file_desc);
	return data[0]>>7 ==1 ? RADIO_MODE_STEREO:RADIO_MODE_MONO;
}

static void tef665x_stop(void)
{
	int ret;
	GstEvent *event;
	audio_set_mute(file_desc, 1);

	 if(initialized && running) {
		// Stop pipeline
		running = false;
		ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
		_debug("gst_element_set_state to pause", ret);

		// Flush pipeline
		// This seems required to avoidstatic  stutters on starts after a stop
		event = gst_event_new_flush_start();
		gst_element_send_event(GST_ELEMENT(pipeline), event);
		event = gst_event_new_flush_stop(TRUE);
		gst_element_send_event(GST_ELEMENT(pipeline), event);
	}
}

static int tef665x_probe()
{
	int rc;

	if(present)
		return 0;

	rc = i2c_init(I2C_DEV, _open, &file_desc);
	if(rc < 0) {
		fprintf(stderr, "tef665x not present");
		return -1;
	}
	_debug("file_desc= ", file_desc);

	rc = appl_get_identification(file_desc);
	if(rc != 1){
		fprintf(stderr, "no tef665x!");
		return -1;
	}

	present = true;
	return 0;
}

static int tef665x_init()
{
	char gst_pipeline_str[GST_PIPELINE_LEN];
	int rc;

	if(!present)
		return -1;

	if(initialized)
		return 0;

	current_am_frequency = known_am_band_plans[am_bandplan].min;
	current_fm_frequency = known_fm_band_plans[fm_bandplan].min;

	current_band = RADIO_BAND_AM;

	radio_powerSwitch(file_desc, 1);

	tef665x_chip_init(file_desc);

 	// Initialize GStreamer
	gst_init(NULL, NULL);

	// Use PipeWire output
	// This pipeline is working on imx6solo, the important thing, up to now, is that it gets xrun error every few seconds.
	// I believe it's related to wireplumber on imx6.
	rc = snprintf(gst_pipeline_str,
		      	GST_PIPELINE_LEN,
				 "alsasrc device=hw:1,0 ! audioconvert ! audioresample ! audio/x-raw, rate=48000, channels=2 \
				 ! pipewiresink stream-properties=\"p,media.role=Multimedia\"");

	if(rc >= GST_PIPELINE_LEN) {
		fprintf(stderr, "pipeline string too long");
		return -1;
	}
	printf("pipeline: , %s\n", gst_pipeline_str);

	pipeline = gst_parse_launch(gst_pipeline_str, NULL);
	if(!pipeline) {
		fprintf(stderr, "pipeline construction failed!");
		return -1;
	}

	// Start pipeline in paused state
	rc = gst_element_set_state(pipeline, GST_STATE_PAUSED);
	_debug("gst_element_set_state to pause (at the begining)", rc);

	rc = gst_bus_add_watch(gst_element_get_bus(pipeline), (GstBusFunc) handle_message, NULL);
	_debug("gst_bus_add_watch   rc", rc);

	//Initialize Mutex Lock for Scan and RDS
	pthread_mutex_init(&scan_mutex, NULL);
	pthread_mutex_init (&RDS_Mutex, NULL);

	initialized = true;

	tef665x_start();
	return 0;
}

static void tef665x_set_frequency_callback(radio_freq_callback_t callback,
				      void *data)
{
	freq_callback = callback;
	freq_callback_data = data;
}
static void tef665x_set_rds_callback(radio_rds_callback_t callback)
{
	rds_callback = callback;
	
}
static void tef665x_set_output(const char *output)
{
}

static radio_band_t tef665x_get_band(void)
{
	_debug("band", current_band);
	return current_band;
}

static void tef665x_set_band(radio_band_t band)
{
	uint32_t fd = 0;
	int ret = i2c_init(I2C_DEV, _open, &fd);

	_debug("i2c_init ret value", ret);

	if(band == RADIO_BAND_FM){
		current_band = band;
		FM_tune_to(fd, eAR_TuningAction_Preset, current_fm_frequency / 10000);
	} else {
		current_band = band;
		AM_tune_to(fd, eAR_TuningAction_Preset, current_am_frequency / 1000);
	}

	i2c_init(I2C_DEV, _close, &fd);

	_debug("band", current_band);
}

static uint32_t tef665x_get_frequency(void)
{
	if(current_band == RADIO_BAND_FM){
		return current_fm_frequency;
	} else {
		return current_am_frequency;
	}
}

static void tef665x_set_alternative_frequency(uint32_t frequency)
{
	uint32_t fd = 0;
	int ret = i2c_init(I2C_DEV, _open, &fd);
	
	if(current_band == RADIO_BAND_FM)
	{
		FM_tune_to(fd, eAR_TuningAction_AF_Update, frequency / 10000);
	}
	
	i2c_init(I2C_DEV, _close, &fd);
}

static void tef665x_set_frequency(uint32_t frequency)
{
	uint32_t fd = 0;

	if(!initialized)
		return;

	if(scanning)
		return;

	if(current_band == RADIO_BAND_FM) {
		if(frequency < known_fm_band_plans[fm_bandplan].min ||
	    	frequency > known_fm_band_plans[fm_bandplan].max ) {
			_debug("invalid FM frequency", frequency);
			return;
			}
	} else {
		if(frequency < known_am_band_plans[am_bandplan].min ||
	   		frequency > known_am_band_plans[am_bandplan].max ) {
			_debug("invalid AM frequency", frequency);
			return;
		}
	}

	int ret = i2c_init(I2C_DEV, _open, &fd);

	if(current_band == RADIO_BAND_FM){
		current_fm_frequency = frequency;
	   _debug("frequency set to FM :", frequency);
		FM_tune_to(fd, eAR_TuningAction_Preset, frequency / 10000);
	} else {
		current_am_frequency = frequency;
	   _debug("frequency set to AM :", frequency);
		AM_tune_to(fd, eAR_TuningAction_Preset, frequency / 1000);
	}
	i2c_init(I2C_DEV, _close, &fd);

	//Send Frequency data to subscribers
	if(freq_callback)
	{
		freq_callback(frequency, freq_callback_data);
	}

	//Start RDS if the band was FM
	if(current_band==RADIO_BAND_FM){
		//Unlock Mutex
		pthread_mutex_unlock (&RDS_Mutex);
		
		//Clean RDS Message
		Clear_RDS_Data(&RDS_Message);

		//Wait to make sure rds thread is finished
		usleep(300000);

		//Restart RDS
		tef665x_get_rds_info();
	}
}

/*
 * @brief Tune to a frequency in search mode
 * 
 * Tune to new program and stay muted
 * Sending new frequency to subscribers
 * 
 * @param uint32_t which is the frequecy to be tuned
 * @return void
 */
static void tef665x_search_frequency(uint32_t frequency)
{
	uint32_t fd = 0;
	int ret = i2c_init(I2C_DEV, _open, &fd);
	if(current_band == RADIO_BAND_FM)
	{
		current_fm_frequency = frequency;
	    _debug("frequency set to FM :", frequency);
		FM_tune_to(fd, eAR_TuningAction_Search, frequency / 10000);
		
	} 
	else 
	{
		current_am_frequency = frequency;
	    _debug("frequency set to AM :", frequency);
		AM_tune_to(fd, eAR_TuningAction_Search, frequency / 1000);
	}
	i2c_init(I2C_DEV, _close, &fd);

	//Send Frequency data to subscribers
	if(freq_callback)
	{
		freq_callback(frequency, freq_callback_data);
	}
}

static int tef665x_band_supported(radio_band_t band)
{
	if(band == RADIO_BAND_FM || band == RADIO_BAND_AM)
		return 1;
	return 0;
}

static uint32_t tef665x_get_min_frequency(radio_band_t band)
{
	if(band == RADIO_BAND_FM) {
		return known_fm_band_plans[fm_bandplan].min;
	} else {
		return known_am_band_plans[am_bandplan].min;
	}
}

static uint32_t tef665x_get_max_frequency(radio_band_t band)
{
	if(band == RADIO_BAND_FM) {
		return known_fm_band_plans[fm_bandplan].max;
	} else {
		return known_am_band_plans[am_bandplan].max;
	}
}

static uint32_t tef665x_get_frequency_step(radio_band_t band)
{
	uint32_t ret = 0;

	switch (band) {
	case RADIO_BAND_AM:
		ret = known_am_band_plans[am_bandplan].step;
		break;
	case RADIO_BAND_FM:
		ret = known_fm_band_plans[fm_bandplan].step;
		break;
	default:
		break;
	}
	return ret;
}

radio_impl_ops_t tef665x_impl_ops = {
	.name = "TEF665x",
	.probe = tef665x_probe,
	.init = tef665x_init,
	.start = tef665x_start,
	.stop = tef665x_stop,
	.set_output = tef665x_set_output,
	.get_frequency = tef665x_get_frequency,
	.set_frequency = tef665x_set_frequency,
	.set_frequency_callback = tef665x_set_frequency_callback,
	.set_rds_callback=tef665x_set_rds_callback,
	.get_band = tef665x_get_band,
	.set_band = tef665x_set_band,
	.band_supported = tef665x_band_supported,
	.get_min_frequency = tef665x_get_min_frequency,
	.get_max_frequency = tef665x_get_max_frequency,
	.get_frequency_step = tef665x_get_frequency_step,
	.scan_start = tef665x_scan_start,
	.scan_stop = tef665x_scan_stop,
	.get_stereo_mode = tef665x_get_stereo_mode,
	//.set_stereo_mode = tef665x_set_stereo_mode,*/
	.get_rds_info = tef665x_get_rds_info,
	.get_quality_info = tef665x_get_quality_info,
	.set_alternative_frequency  = tef665x_set_alternative_frequency
};
