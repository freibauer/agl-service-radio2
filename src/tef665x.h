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

#ifndef TEF665X_H
#define TEF665X_H

typedef uint8_t u8;
typedef uint16_t ushort;
typedef uint32_t uint;

typedef enum
{
_close = 0,
_open
} I2C_STATE;

typedef enum
{
	TEF665X_MODULE_FM = 0x20,      //32
	TEF665X_MODULE_AM = 0x21,	    //31
	TEF665X_MODULE_AUDIO = 0x30,   //48
	TEF665X_MODULE_APPL  = 0x40    //64
} TEF665x_MODULE;

enum {
	TEF6657_CMD_tune = 0,
	TEF6657_CMD_open = 1,
	TEF6657_CMD_close = 2,
	TEF6657_CMD_am = 3,
	TEF6657_CMD_fm = 4,
	TEF6657_CMD_GETamSTAUS  = 5,
	TEF6657_CMD_GETfmSTAUS  = 6,
	TEF6657_CMD_GEToirtSTAUS
};

enum
{
	RADIO_BOOT_STATE = 0,
	RADIO_IDLE_STATE,
	RADIO_STANBDY_STATE,
	RADIO_FM_STATE,
	RADIO_AM_STATE

};

typedef enum
{
	TEF665X_Cmd_Set_OperationMode = 0x01,
	TEF665X_Cmd_Set_GPIO = 0x03,
	TEF665X_Cmd_Set_ReferenceClock = 0x04,
	TEF665X_Cmd_Activate = 0x05,

	TEF665X_Cmd_Get_Operation_Status = 0x80, //128,
	TEF665X_Cmd_Get_GPIO_Status = 0x81,  //129,
	TEF665X_Cmd_Get_Identification = 0x82,  //130,
	TEF665X_Cmd_Get_LastWrite = 0x83, //131
} TEF665x_APPL_COMMAND;

typedef enum
{
	TEF665X_Cmd_Set_RDS_mode = 0x01, // default
	TEF665X_Cmd_Set_RDS_autorestart= 0x02, // restart after tune
	TEF665X_Cmd_Set_RDS_interface = 0, // no interface
} TEF665x_FM_COMMAND;

typedef enum
{
	TEF665X_Cmd_Tune_To  =  0x01,
	TEF665X_Cmd_Set_Tune_Options   =0x02,
	TEF665X_Cmd_Set_Bandwidth      =0x0A,     //10,
	TEF665X_Cmd_Set_RFAGC            =0x0B,   //11,
	TEF665X_Cmd_Set_Antenna =0x0C,            //12,

	TEF665X_Cmd_Set_MphSuppression =0x14,     //20,
	TEF665X_Cmd_Set_NoiseBlanker =0x17,       //23,
	TEF665X_Cmd_Set_NoiseBlanker_Audio =0x18, //24,

	TEF665X_Cmd_Set_DigitalRadio =0x1E,       //30,
	TEF665X_Cmd_Set_Deemphasis =0x1F,         //31,

	TEF665X_Cmd_Set_LevelStep = 0x26,         //38,
	TEF665X_Cmd_Set_LevelOffset = 0x27,       //39,

	TEF665X_Cmd_Set_Softmute_Time =0x28,      //40,
	TEF665X_Cmd_Set_Softmute_Mod = 0x29,      //41,
	TEF665X_Cmd_Set_Softmute_Level =0x2A,     //42,
	TEF665X_Cmd_Set_Softmute_Noise =0x2B,     //43,
	TEF665X_Cmd_Set_Softmute_Mph =0x2C,       //44,
	TEF665X_Cmd_Set_Softmute_Max =0x2D,       //45,

	TEF665X_Cmd_Set_Highcut_Time =0x32,       //50,
	TEF665X_Cmd_Set_Highcut_Mod = 0x33,       //51,
	TEF665X_Cmd_Set_Highcut_Level =0x34,      //52,
	TEF665X_Cmd_Set_Highcut_Noise = 0x35,     //53,
	TEF665X_Cmd_Set_Highcut_Mph =0x36,        //54,
	TEF665X_Cmd_Set_Highcut_Max =0x37,        //55,
	TEF665X_Cmd_Set_Highcut_Min =0x38,        //56,
	TEF665X_Cmd_Set_Lowcut_Min =0x3A,         //58,

	TEF665X_Cmd_Set_Stereo_Time =0x3C,        //60,
	TEF665X_Cmd_Set_Stereo_Mod =0x3D,         //61,
	TEF665X_Cmd_Set_Stereo_Level =0x3E,       //62,
	TEF665X_Cmd_Set_Stereo_Noise = 0x3F,      //63,
	TEF665X_Cmd_Set_Stereo_Mph =0x40,         //64,
	TEF665X_Cmd_Set_Stereo_Max = 0x41,        //65,
	TEF665X_Cmd_Set_Stereo_Min = 0x42,        //66,

	TEF665X_Cmd_Set_Scaler = 0x50,            //80,
	TEF665X_Cmd_Set_RDS = 0x51,               //81,
	TEF665X_Cmd_Set_QualityStatus = 0x52,     //82,
	TEF665X_Cmd_Set_DR_Blend =  0x53,         //83,
	TEF665X_Cmd_Set_DR_Options =  0x54,       //84,
	TEF665X_Cmd_Set_Specials = 0x55,          //85,

	TEF665X_Cmd_Get_Quality_Status = 0x80,    //128,
	TEF665X_Cmd_Get_Quality_Data = 0x81,      //129,
	TEF665X_Cmd_Get_RDS_Status = 0x82,        //130,
	TEF665X_Cmd_Get_RDS_Data = 0x83,          //131,
	TEF665X_Cmd_Get_AGC = 0x84,               //132,
	TEF665X_Cmd_Get_Signal_Status =   0x85,   //133,
	TEF665X_Cmd_Get_Processing_Status = 0x86, //134,
	TEF665X_Cmd_Get_Interface_Status = 0x87,  //135,

	TEF665X_Cmd_Set_Output_signal_i2s = 0x21,          //33,
	TEF665X_Cmd_Set_Output_signal_dac = 0x80,          //128,
	TEF665X_Cmd_Set_Output_source_aRadio = 0x04,       //4,
	TEF665X_Cmd_Set_Output_source_dInput = 0x20,       //32,
	TEF665X_Cmd_Set_Output_source_aProcessor = 0xe0,   //224,
	TEF665X_Cmd_Set_Output_source_SinWave = 0xf0,      //240,
	} TEF665x_RADIO_COMMAND;

typedef enum
{
	TEF665X_Cmd_Set_Volume = 0x0A,  //10,
	TEF665X_Cmd_Set_Mute = 0x0B,  //11,
	TEF665X_Cmd_Set_Input = 0x0C,  //12,
	TEF665X_Cmd_Set_Output_Source = 0x0D,  //13,

	TEF665X_Cmd_Set_Ana_Out = 0x15,  //21,
	TEF665X_Cmd_Set_Dig_IO = 0x16,  //22,
	TEF665X_Cmd_Set_Input_Scaler = 0x17,  //23,
	TEF665X_Cmd_Set_WaveGen = 0x18,  //24
} TEF665x_AUDIO_COMMAND;

typedef enum
{
   TEF665X_AUDIO_CMD_22_SIGNAL_i2s1 = 0x21,       //33,
   TEF665X_AUDIO_CMD_22_MODE_voltage = 0x02,      //2,
   TEF665X_AUDIO_CMD_22_FORMAT_16 = 0x10,         //16,
   TEF665X_AUDIO_CMD_22_FORMAT_32 = 0x20,         //32,
   TEF665X_AUDIO_CMD_22_OPERATION_slave = 0,      //0,
   TEF665X_AUDIO_CMD_22_OPERATION_master = 0x0100,//256,
   TEF665X_AUDIO_CMD_22_SAMPLERATE_48K = 0x12c0   //4800
} TEF665x_AUDIO_CMD_22;

typedef enum{
	eDevTEF665x_Boot_state ,
	eDevTEF665x_Idle_state,
	eDevTEF665x_Wait_Active,
	eDevTEF665x_Active_state,
   
   eDevTEF665x_Power_on,
	
   eDevTEF665x_Not_Exist,

	eDevTEF665x_Last
}TEF665x_STATE;

const u8 patchByteValues[]=
{
   0xF0, 0x00, 0x38, 0x16, 0xD0, 0x80, 0x43, 0xB2, 0x38, 0x1D, 0xD0, 0x80, 0xF0, 0x00, 0x70, 0x00, 0xC2, 0xF7, 0xF0, 0x00, 0x38, 0x4E, 0xD0, 0x80,
   0xF0, 0x00, 0x38, 0xF1, 0xD0, 0x80, 0xC4, 0xA2, 0x02, 0x0C, 0x60, 0x04, 0x90, 0x01, 0x39, 0x07, 0xD0, 0x80, 0xF0, 0x00, 0x38, 0xD3, 0xD0, 0x80,
   0xF0, 0x00, 0x39, 0x0E, 0xD2, 0x80, 0xF0, 0x00, 0x39, 0x12, 0xD0, 0x80, 0x40, 0x20, 0x39, 0x1E, 0xD0, 0x80, 0x9E, 0x30, 0x18, 0xF9, 0xD2, 0x80,
   0xF0, 0x00, 0x39, 0x20, 0xD0, 0x80, 0xF0, 0x00, 0x39, 0x23, 0xD0, 0x80, 0xF0, 0x00, 0x39, 0x38, 0xD0, 0x80, 0xF0, 0x00, 0x39, 0x3B, 0xD0, 0x80,
   0x00, 0x43, 0x39, 0x43, 0xD9, 0x80, 0xF0, 0x00, 0x39, 0x46, 0xD0, 0x80, 0xF0, 0x00, 0x39, 0x60, 0xD0, 0x80, 0xF0, 0x00, 0x39, 0x71, 0xD0, 0x80,
   0xF0, 0x00, 0x39, 0x7F, 0xD0, 0x80, 0xF0, 0x00, 0x39, 0x82, 0xD0, 0x80, 0xF0, 0x00, 0x70, 0x00, 0xA0, 0x14, 0xF0, 0x00, 0x70, 0x00, 0xA0, 0xD4,
   0xF0, 0x00, 0x70, 0x00, 0xA0, 0xDB, 0xF0, 0x00, 0x70, 0x00, 0xA1, 0x0C, 0xF0, 0x00, 0x70, 0x00, 0xA1, 0x12, 0xF0, 0x00, 0x70, 0x00, 0xA1, 0x2D,
   0xF0, 0x00, 0x20, 0x31, 0xD0, 0x80, 0x00, 0x7F, 0x60, 0x02, 0xE2, 0x00, 0xF0, 0x00, 0x0E, 0x22, 0x60, 0x0A, 0xF0, 0x00, 0x00, 0xFF, 0x60, 0x03,
   0xF0, 0x00, 0x01, 0x42, 0xD2, 0x80, 0x90, 0x03, 0x40, 0x02, 0xF0, 0x00, 0x90, 0x43, 0x01, 0x70, 0xD1, 0x80, 0xF0, 0x00, 0x01, 0x69, 0xD0, 0x80,
   0x0E, 0x69, 0x60, 0x0A, 0xA1, 0x60, 0x20, 0x23, 0x00, 0x01, 0x60, 0x01, 0xF0, 0x00, 0x70, 0x00, 0xF0, 0x00, 0xC4, 0xCB, 0x70, 0x00, 0xF0, 0x00,
   0xCA, 0x09, 0x30, 0x23, 0xF0, 0x00, 0xC2, 0xCB, 0x70, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x30, 0x23, 0xD0, 0x08, 0x82, 0x00, 0x0D, 0x50, 0x60, 0x08,
   0xF0, 0x00, 0x0D, 0x51, 0x60, 0x09, 0x30, 0x00, 0x21, 0x80, 0x60, 0x01, 0xF0, 0x00, 0x40, 0x32, 0xF0, 0x00, 0x30, 0x11, 0x45, 0xF3, 0xF0, 0x00,
   0x30, 0x92, 0x2D, 0x30, 0x60, 0x04, 0x31, 0x13, 0x2D, 0x40, 0x60, 0x05, 0x31, 0x94, 0x7F, 0xFF, 0x60, 0x06, 0x32, 0x15, 0x0D, 0x61, 0x60, 0x0A,
   0x32, 0x96, 0x0D, 0x6B, 0x60, 0x0B, 0x33, 0x10, 0x0D, 0x50, 0x60, 0x01, 0x33, 0x90, 0x0D, 0x5C, 0x60, 0x02, 0x30, 0x21, 0x0D, 0x63, 0x60, 0x03,
   0x30, 0x31, 0x0D, 0x75, 0x60, 0x0C, 0x30, 0xA2, 0x8D, 0x00, 0x60, 0x01, 0x30, 0xB3, 0x01, 0x73, 0x60, 0x02, 0x30, 0x41, 0x00, 0x25, 0x60, 0x03,
   0x30, 0xC2, 0x40, 0x44, 0xF0, 0x00, 0x31, 0x43, 0x40, 0x35, 0xF0, 0x00, 0x31, 0xC4, 0x64, 0x00, 0x60, 0x06, 0x32, 0x45, 0x1F, 0x40, 0x60, 0x07,
   0x32, 0xC6, 0x70, 0x00, 0xF0, 0x00, 0x33, 0x47, 0x1E, 0xBC, 0x60, 0x0D, 0x33, 0xC0, 0x01, 0x22, 0x60, 0x01, 0x34, 0x40, 0xFD, 0xEE, 0x60, 0x02,
   0x30, 0x51, 0x7B, 0x8F, 0x60, 0x03, 0x30, 0xD2, 0xC4, 0x29, 0x60, 0x04, 0x31, 0x51, 0x1E, 0xC2, 0x60, 0x0E, 0x32, 0x53, 0xFF, 0x0D, 0x60, 0x02,
   0x32, 0xD4, 0x7D, 0x2E, 0x60, 0x03, 0x30, 0x61, 0xC1, 0x9A, 0x60, 0x04, 0x30, 0xE2, 0x70, 0x00, 0xF0, 0x00, 0x31, 0x61, 0x70, 0x00, 0xF0, 0x00,
   0x32, 0x63, 0x70, 0x00, 0xF0, 0x00, 0x32, 0xE4, 0x70, 0x00, 0xD0, 0x08, 0xF0, 0x00, 0x03, 0x70, 0xD2, 0x80, 0xF0, 0x00, 0x70, 0x00, 0xA0, 0x02,
   0xF0, 0x00, 0x70, 0x00, 0xA0, 0x59, 0xF0, 0x00, 0x02, 0x15, 0xD0, 0x80, 0xF0, 0x00, 0x0D, 0x51, 0x60, 0x0F, 0xF0, 0x00, 0x05, 0x17, 0x60, 0x0E,
   0x23, 0xF6, 0x70, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x21, 0x63, 0x41, 0xF5, 0x91, 0x8F, 0x21, 0xF8, 0x40, 0x74, 0xC3, 0xEF, 0x21, 0xE0, 0xF0, 0x00,
   0xC3, 0xA4, 0x33, 0xF7, 0xF0, 0x00, 0xD8, 0x5B, 0x70, 0x00, 0xF0, 0x00, 0x82, 0x18, 0x70, 0x00, 0xF0, 0x00, 0x9F, 0xAF, 0x18, 0x00, 0xF0, 0x00,
   0x9F, 0x0F, 0x31, 0xF8, 0x90, 0x02, 0xF0, 0x00, 0x70, 0x00, 0x90, 0x28, 0xF0, 0x00, 0x70, 0x00, 0xD0, 0x08, 0xF0, 0x00, 0x22, 0x78, 0xF0, 0x00,
   0x16, 0xD3, 0x60, 0x09, 0xA0, 0x7F, 0x35, 0xF0, 0x1E, 0xBC, 0x60, 0x0D, 0xF0, 0x00, 0x0D, 0x61, 0x60, 0x08, 0xF0, 0x00, 0x03, 0xA5, 0xD2, 0x80,
   0xF0, 0x00, 0x1E, 0xC2, 0x60, 0x0D, 0xF0, 0x00, 0x0D, 0x6B, 0x60, 0x08, 0xF0, 0x00, 0x03, 0xA5, 0xD2, 0x80, 0xF0, 0x00, 0x21, 0x00, 0xF0, 0x00,
   0x83, 0x6D, 0x22, 0xF1, 0xF0, 0x00, 0xF0, 0x00, 0x23, 0x77, 0xF0, 0x00, 0x90, 0x41, 0x36, 0x70, 0xF0, 0x00, 0x9E, 0x79, 0x70, 0x00, 0x90, 0x01,
   0xF0, 0x00, 0x32, 0xF1, 0xD0, 0x08, 0x91, 0xC7, 0x33, 0x75, 0xF0, 0x00, 0xF0, 0x00, 0x34, 0x70, 0xE6, 0x00, 0xF0, 0x00, 0x34, 0xF0, 0xE6, 0x00,
   0xF0, 0x00, 0x24, 0x74, 0xF0, 0x00, 0xF0, 0x00, 0x24, 0xF3, 0xF0, 0x00, 0x8C, 0x24, 0x26, 0xF2, 0x40, 0x16, 0x8A, 0x1B, 0x34, 0x74, 0x4F, 0xF5,
   0x82, 0xB7, 0x34, 0xF3, 0xF0, 0x00, 0xF0, 0x00, 0x20, 0x71, 0x90, 0x05, 0x83, 0x04, 0x70, 0x00, 0xF0, 0x00, 0x8E, 0x67, 0x70, 0x00, 0xF0, 0x00,
   0xF0, 0x00, 0x70, 0x00, 0x90, 0x02, 0xF0, 0x00, 0x36, 0xF6, 0xF0, 0x00, 0xF0, 0x00, 0x34, 0xF0, 0x80, 0x06, 0x82, 0xAF, 0x70, 0x00, 0xF0, 0x00,
   0x82, 0x1B, 0x70, 0x00, 0xD0, 0x09, 0x8E, 0x5F, 0x70, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x70, 0x00, 0xD0, 0x09, 0xF0, 0x00, 0x36, 0xF5, 0xF0, 0x00,
   0xF0, 0x00, 0x34, 0x70, 0xF0, 0x00, 0x40, 0x11, 0x27, 0x72, 0xA1, 0x03, 0x90, 0x8A, 0x20, 0xF3, 0xA1, 0x02, 0x8E, 0xD7, 0x37, 0x72, 0xF0, 0x00,
   0xF0, 0x00, 0x37, 0xF1, 0xE6, 0x00, 0xF0, 0x00, 0x70, 0x00, 0xD0, 0x08, 0xF0, 0x00, 0x22, 0x7A, 0xF0, 0x00, 0x16, 0xC3, 0x60, 0x09, 0xA0, 0x58,
   0xF0, 0x00, 0x18, 0x20, 0xF0, 0x00, 0xF0, 0x00, 0x35, 0x70, 0xF0, 0x00, 0xF0, 0x00, 0x32, 0x7A, 0xD0, 0x08, 0x82, 0x00, 0x0D, 0x51, 0x60, 0x08,
   0x40, 0x03, 0x70, 0x00, 0xF0, 0x00, 0x33, 0x80, 0x70, 0x00, 0xF0, 0x00, 0x21, 0x06, 0x70, 0x00, 0xF0, 0x00, 0x37, 0x00, 0x70, 0x00, 0xF0, 0x00,
   0x37, 0x80, 0x40, 0x15, 0xF0, 0x00, 0x36, 0x83, 0x70, 0x00, 0xF0, 0x00, 0x33, 0x05, 0x0D, 0x61, 0x60, 0x09, 0x32, 0x86, 0x0D, 0x6B, 0x60, 0x0A,
   0x32, 0x10, 0x70, 0x00, 0xF0, 0x00, 0x32, 0x90, 0x70, 0x00, 0xF0, 0x00, 0x33, 0x10, 0x70, 0x00, 0xF0, 0x00, 0x33, 0x90, 0x70, 0x00, 0xF0, 0x00,
   0x34, 0x10, 0x70, 0x00, 0xF0, 0x00, 0x34, 0x90, 0x70, 0x00, 0xF0, 0x00, 0x31, 0x10, 0x70, 0x00, 0xF0, 0x00, 0x31, 0x90, 0x70, 0x00, 0xF0, 0x00,
   0x32, 0x20, 0x70, 0x00, 0xF0, 0x00, 0x32, 0xA0, 0x70, 0x00, 0xF0, 0x00, 0x33, 0x20, 0x70, 0x00, 0xF0, 0x00, 0x33, 0xA0, 0x70, 0x00, 0xF0, 0x00,
   0x34, 0x20, 0x70, 0x00, 0xF0, 0x00, 0x34, 0xA0, 0x70, 0x00, 0xF0, 0x00, 0x31, 0x20, 0x70, 0x00, 0xF0, 0x00, 0x31, 0xA0, 0x70, 0x00, 0xF0, 0x00,
   0x82, 0x00, 0x0D, 0x30, 0x60, 0x0A, 0x0D, 0x40, 0x60, 0x0B, 0xC0, 0x10, 0xF0, 0x00, 0x10, 0x20, 0xF0, 0x00, 0x0D, 0x51, 0x60, 0x0C, 0xC0, 0x10,
   0xF0, 0x00, 0x10, 0x30, 0xF0, 0x00, 0xF0, 0x00, 0x35, 0xC0, 0xD0, 0x08, 0xF0, 0x00, 0x0D, 0x75, 0x60, 0x0F, 0xF0, 0x00, 0x05, 0x63, 0x60, 0x0E,
   0x24, 0xF7, 0x05, 0x1D, 0x60, 0x0D, 0x25, 0x76, 0x70, 0x00, 0xF0, 0x00, 0x91, 0xC7, 0x20, 0xE8, 0x40, 0x15, 0x91, 0x8F, 0x21, 0xE9, 0xD4, 0x09,
   0xC3, 0xEF, 0x20, 0x00, 0x40, 0x12, 0x9F, 0xBE, 0x20, 0x11, 0x58, 0x03, 0xA0, 0x80, 0x35, 0x77, 0x90, 0x01, 0xF0, 0x00, 0x70, 0x00, 0xD0, 0x08,
   0xF0, 0x00, 0x21, 0xF5, 0xF0, 0x00, 0xA0, 0xCA, 0x22, 0x54, 0xF0, 0x00, 0xCC, 0x09, 0x05, 0x17, 0x60, 0x0C, 0x83, 0x2C, 0x70, 0x00, 0xF0, 0x00,
   0x8A, 0x61, 0x70, 0x00, 0xF0, 0x00, 0xAE, 0x48, 0x22, 0x45, 0xA0, 0xCB, 0xA2, 0x28, 0x20, 0x78, 0xF0, 0x00, 0xF0, 0x00, 0x35, 0xF0, 0xF0, 0x00,
   0xF0, 0x00, 0x18, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x30, 0x78, 0xF0, 0x00, 0x16, 0xE3, 0x60, 0x09, 0xA0, 0x27, 0x89, 0x01, 0x23, 0xF4, 0xF0, 0x00,
   0xF0, 0x00, 0x20, 0xF2, 0xF0, 0x00, 0x82, 0x61, 0x21, 0x73, 0xF0, 0x00, 0xA0, 0x50, 0x36, 0x70, 0xF0, 0x00, 0xA0, 0x58, 0x23, 0x72, 0xE1, 0x40,
   0xA8, 0x01, 0x22, 0xF3, 0xF0, 0x00, 0x90, 0x49, 0x22, 0x75, 0xE0, 0x40, 0x80, 0x61, 0x70, 0x00, 0xF0, 0x00, 0x8A, 0x51, 0x33, 0xF1, 0xF0, 0x00,
   0xA0, 0x58, 0x70, 0x00, 0xF0, 0x00, 0xAF, 0x48, 0x70, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x34, 0x70, 0xD0, 0x08, 0x82, 0x00, 0x0D, 0x75, 0x60, 0x08,
   0x90, 0x09, 0x0D, 0x00, 0x60, 0x09, 0xF0, 0x00, 0x35, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x33, 0x80, 0xC0, 0x28, 0xF0, 0x00, 0x10, 0x10, 0xF0, 0x00,
   0xF0, 0x00, 0x34, 0x81, 0xD0, 0x08, 0x82, 0x49, 0x0D, 0x75, 0x60, 0x08, 0xF0, 0x00, 0x70, 0x00, 0x8F, 0xFD, 0x04, 0x00, 0x60, 0x00, 0xA0, 0xB1,
   0x8E, 0xC0, 0x40, 0x00, 0x60, 0x05, 0x60, 0x00, 0x60, 0x05, 0xE6, 0x00, 0xC8, 0x1B, 0x70, 0x00, 0xF0, 0x00, 0xD8, 0xDB, 0x0D, 0x51, 0x60, 0x08,
   0x83, 0x5B, 0x70, 0x00, 0xF0, 0x00, 0x9E, 0xBA, 0x30, 0x03, 0xF0, 0x00, 0xF0, 0x00, 0x30, 0x84, 0xD4, 0x09, 0xF0, 0x00, 0x70, 0x00, 0x8F, 0xAF,
   0xF0, 0x00, 0x0D, 0x75, 0x60, 0x08, 0xF0, 0x00, 0x0D, 0x51, 0x60, 0x09, 0xF0, 0x00, 0x24, 0x03, 0xF0, 0x00, 0xF0, 0x00, 0x27, 0x94, 0xD0, 0x08,
   0xA0, 0x03, 0x70, 0x00, 0xF0, 0x00, 0x00, 0x11, 0x08, 0x00, 0xF0, 0x00, 0x00, 0x11, 0x08, 0x00, 0xC0, 0x0E, 0xA0, 0x09, 0x00, 0x11, 0x08, 0x00,
   0xA0, 0x09, 0x70, 0x00, 0xF0, 0x00, 0xA4, 0x08, 0x70, 0x00, 0xD0, 0x08, 0xA0, 0x03, 0x70, 0x00, 0xF0, 0x00, 0x00, 0x11, 0x08, 0x00, 0xF0, 0x00,
   0x00, 0x11, 0x08, 0x00, 0xC0, 0x26, 0xA0, 0x09, 0x00, 0x11, 0x08, 0x00, 0xA0, 0x09, 0x70, 0x00, 0xF0, 0x00, 0xA4, 0x08, 0x70, 0x00, 0xD0, 0x08,
   0xF0, 0x00, 0x1D, 0x01, 0x60, 0x08, 0xF0, 0x00, 0x0A, 0x2C, 0x60, 0x00, 0xF0, 0x00, 0x01, 0x1A, 0x60, 0x01, 0x31, 0x00, 0x70, 0x00, 0xF0, 0x00,
   0x31, 0x81, 0x70, 0x00, 0xD0, 0x08, 0x10, 0x00, 0x60, 0x03, 0xA0, 0x93, 0x30, 0x23, 0x07, 0x73, 0xD2, 0x80, 0xF0, 0x00, 0x07, 0xC6, 0xD0, 0x80,
   0x40, 0xE0, 0x00, 0x1F, 0x60, 0x01, 0x13, 0xD5, 0x60, 0x07, 0xA0, 0x06, 0x13, 0xFB, 0x60, 0x06, 0xF0, 0x00, 0x90, 0x40, 0x0D, 0x28, 0xD2, 0x80,
   0x14, 0x05, 0x60, 0x06, 0xF0, 0x00, 0xF0, 0x00, 0x0D, 0x28, 0xD2, 0x80, 0x14, 0x0F, 0x60, 0x06, 0xF0, 0x00, 0xF0, 0x00, 0x0D, 0x28, 0xD0, 0x80,
   0xD7, 0xCA, 0x00, 0xFF, 0x60, 0x04, 0x81, 0xD7, 0x0C, 0xF7, 0x60, 0x09, 0xD0, 0x56, 0x70, 0x00, 0xF0, 0x00, 0x82, 0x76, 0x30, 0x17, 0xF0, 0x00,
   0xD0, 0xF6, 0x40, 0x83, 0xF0, 0x00, 0xC1, 0xA4, 0x20, 0x19, 0xF0, 0x00, 0x82, 0xF6, 0x70, 0x00, 0xF0, 0x00, 0xC1, 0x80, 0x20, 0x17, 0xA0, 0x81,
   0xC3, 0xE7, 0x70, 0x00, 0xF0, 0x00, 0xC5, 0xC7, 0x70, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x10, 0xB1, 0xD0, 0x80, 0x40, 0x00, 0x70, 0x00, 0xAF, 0xF0,
   0x41, 0xF1, 0x40, 0x40, 0xF0, 0x00, 0xF0, 0x00, 0x10, 0xB2, 0xD0, 0x80, 0x40, 0x71, 0x10, 0xB2, 0xD2, 0x80, 0xF0, 0x00, 0x10, 0x81, 0xD0, 0x80,
   0xF0, 0x00, 0x0B, 0xC9, 0x60, 0x08, 0xF0, 0x00, 0x1D, 0x8D, 0xD2, 0x80, 0xF0, 0x00, 0x16, 0xD5, 0xD0, 0x80, 0xF0, 0x00, 0x0B, 0xC9, 0x60, 0x08,
   0xF0, 0x00, 0x1D, 0x8F, 0xD2, 0x80, 0xF0, 0x00, 0x16, 0xDA, 0xD0, 0x80, 0xF0, 0x00, 0x0B, 0x60, 0x60, 0x0E, 0xF0, 0x00, 0x00, 0x04, 0x60, 0x03,
   0xF0, 0x00, 0x3F, 0xFC, 0x60, 0x04, 0x32, 0x63, 0x80, 0x08, 0x60, 0x05, 0x32, 0xE4, 0x19, 0x9A, 0x60, 0x06, 0x33, 0x65, 0x70, 0x00, 0xF0, 0x00,
   0x31, 0xE6, 0x70, 0x00, 0xD0, 0x08, 0x83, 0x6D, 0x0C, 0x35, 0x60, 0x08, 0x40, 0x60, 0x39, 0x36, 0x60, 0x01, 0x41, 0xE2, 0x21, 0x96, 0x60, 0x03,
   0x33, 0x00, 0x41, 0x44, 0xF0, 0x00, 0x33, 0x81, 0x70, 0x00, 0xF0, 0x00, 0x34, 0x02, 0x70, 0x00, 0xF0, 0x00, 0x34, 0x83, 0x70, 0x00, 0xF0, 0x00,
   0x35, 0x04, 0x70, 0x00, 0xF0, 0x00, 0x35, 0x85, 0x70, 0x00, 0xD0, 0x08, 0xF0, 0x00, 0x70, 0x00, 0xAF, 0x54, 0xF0, 0x00, 0x70, 0x00, 0x8F, 0x99,
   0xF0, 0x00, 0x70, 0x00, 0xAF, 0x92, 0xF0, 0x00, 0x0C, 0x51, 0xD2, 0x80, 0xF0, 0x00, 0x21, 0xA0, 0xD0, 0x80, 0xF0, 0x00, 0x05, 0x2E, 0xD2, 0x80,
   0xF0, 0x00, 0x70, 0x00, 0xA0, 0x01, 0xF0, 0x00, 0x21, 0xB7, 0xD0, 0x80, 0xF0, 0x00, 0x20, 0xF0, 0xD2, 0x80, 0x90, 0x02, 0x27, 0xDF, 0xD2, 0x80,
   0x9E, 0x69, 0x70, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x07, 0x00, 0xD1, 0x80, 0xF0, 0x00, 0x23, 0x20, 0xD0, 0x80, 0x17, 0x0B, 0x60, 0x0C, 0xA0, 0x41,
   0x00, 0x40, 0x40, 0x05, 0xF0, 0x00, 0x00, 0x41, 0x24, 0x3F, 0xD0, 0x80, 0xF0, 0x00, 0x70, 0x00, 0xAE, 0xDD, 0xF0, 0x00, 0x0C, 0x8D, 0x60, 0x08,
   0xF0, 0x00, 0x26, 0x0A, 0xD0, 0x80, 0x83, 0xFF, 0x0D, 0x83, 0x60, 0x08, 0xF0, 0x00, 0x01, 0xF4, 0x60, 0x00, 0xF0, 0x00, 0x03, 0xB1, 0x60, 0x01,
   0x10, 0x00, 0x03, 0xB2, 0x60, 0x02, 0x10, 0x01, 0x04, 0x0E, 0x60, 0x00, 0x10, 0x02, 0x04, 0x0F, 0x60, 0x01, 0x10, 0x00, 0x04, 0x5C, 0x60, 0x02,
   0x10, 0x01, 0x04, 0x5D, 0x60, 0x00, 0x10, 0x02, 0x13, 0x80, 0x60, 0x01, 0x10, 0x00, 0x0D, 0x8E, 0x60, 0x09, 0x10, 0x01, 0x02, 0xEE, 0x60, 0x00,
   0x10, 0x07, 0x43, 0x06, 0x60, 0x01, 0x10, 0x10, 0x04, 0x69, 0x60, 0x02, 0x10, 0x11, 0x44, 0x87, 0x60, 0x00, 0x10, 0x12, 0x05, 0xE3, 0x60, 0x01,
   0x10, 0x10, 0x46, 0x08, 0x60, 0x02, 0x10, 0x11, 0x06, 0xAE, 0x60, 0x00, 0x10, 0x12, 0x0D, 0x8C, 0x60, 0x08, 0x10, 0x10, 0x9E, 0x3C, 0x60, 0x00,
   0x10, 0x17, 0x0D, 0x82, 0x60, 0x09, 0x10, 0x00, 0x70, 0x00, 0xF0, 0x00, 0x10, 0x07, 0x70, 0x00, 0xF0, 0x00, 0x10, 0x17, 0x70, 0x00, 0xD0, 0x08,
   0x0D, 0x83, 0x60, 0x08, 0xA0, 0x24, 0xF0, 0x00, 0x00, 0x02, 0xA0, 0x23, 0x90, 0x82, 0x70, 0x00, 0xF0, 0x00, 0x82, 0x8A, 0x70, 0x00, 0x90, 0x03,
   0x90, 0x8A, 0x70, 0x00, 0x90, 0x01, 0xF0, 0x00, 0x70, 0x00, 0x8F, 0xFB, 0x82, 0xBF, 0x70, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x01, 0x99, 0xD2, 0x80,
   0xF0, 0x00, 0x0D, 0x82, 0x60, 0x08, 0xF0, 0x00, 0x26, 0xC1, 0xF0, 0x00, 0xF0, 0x00, 0x00, 0x02, 0xA0, 0x1A, 0x90, 0x82, 0x70, 0x00, 0xF0, 0x00,
   0x82, 0x8A, 0x70, 0x00, 0x90, 0x03, 0x90, 0x8A, 0x70, 0x00, 0x90, 0x01, 0xF0, 0x00, 0x70, 0x00, 0x8F, 0xFB, 0x82, 0x80, 0x70, 0x00, 0xF0, 0x00,
   0xF0, 0x00, 0x70, 0x00, 0xD0, 0x08, 0xF0, 0x00, 0x0D, 0x8E, 0x60, 0x0D, 0xF0, 0x00, 0x3F, 0xFF, 0x60, 0x02, 0xF0, 0x00, 0x00, 0x51, 0xA0, 0x11,
   0xC2, 0x53, 0x70, 0x00, 0xF0, 0x00, 0x8E, 0xC4, 0x70, 0x00, 0x90, 0x01, 0xF0, 0x00, 0x70, 0x00, 0x97, 0xFC, 0xD4, 0x8F, 0x01, 0x99, 0xD2, 0x80,
   0x40, 0x05, 0x0D, 0x8C, 0x60, 0x0D, 0x40, 0x17, 0x3F, 0xFF, 0x60, 0x02, 0x9F, 0x7D, 0x00, 0x51, 0xA0, 0x0A, 0xC2, 0x53, 0x70, 0x00, 0xF0, 0x00,
   0x82, 0xC4, 0x27, 0x1C, 0xD1, 0x80, 0xF0, 0x00, 0x70, 0x00, 0x97, 0xFC, 0x91, 0x46, 0x27, 0x20, 0xD0, 0x80, 0xF0, 0x00, 0x29, 0x15, 0xD2, 0x80,
   0xF0, 0x00, 0x39, 0x86, 0xD2, 0x80, 0xF0, 0x00, 0x28, 0x88, 0xD0, 0x80, 0x01, 0x52, 0x60, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x28, 0xDC, 0xD2, 0x80,
   0xF0, 0x00, 0x28, 0xD5, 0xD0, 0x80, 0xF0, 0x00, 0x70, 0x00, 0xD0, 0x08, 0xF0, 0x00, 0x16, 0xC3, 0x60, 0x08, 0xF0, 0x00, 0x00, 0x9A, 0x60, 0x00,
   0xF0, 0x00, 0x02, 0xE3, 0x60, 0x00, 0x10, 0x00, 0x04, 0xF4, 0x60, 0x00, 0x10, 0x00, 0x06, 0xFF, 0x60, 0x00, 0x10, 0x00, 0x09, 0x07, 0x60, 0x00,
   0x10, 0x00, 0x0B, 0x10, 0x60, 0x00, 0x10, 0x00, 0x0D, 0x1F, 0x60, 0x00, 0x10, 0x00, 0x0F, 0x65, 0x60, 0x00, 0x10, 0x00, 0x0F, 0x65, 0x60, 0x00,
   0x10, 0x00, 0x0D, 0x1F, 0x60, 0x00, 0x10, 0x00, 0x0B, 0x10, 0x60, 0x00, 0x10, 0x00, 0x09, 0x07, 0x60, 0x00, 0x10, 0x00, 0x06, 0xFF, 0x60, 0x00,
   0x10, 0x00, 0x04, 0xF4, 0x60, 0x00, 0x10, 0x00, 0x02, 0xE3, 0x60, 0x00, 0x10, 0x00, 0x00, 0x9A, 0x60, 0x00, 0xF0, 0x00, 0x10, 0x00, 0xF0, 0x00,
   0xF0, 0x00, 0x10, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x16, 0xD3, 0x60, 0x08, 0xF0, 0x00, 0xFF, 0x93, 0x60, 0x00, 0xF0, 0x00, 0xFE, 0x37, 0x60, 0x00,
   0x10, 0x00, 0xFD, 0xD9, 0x60, 0x00, 0x10, 0x00, 0xFE, 0x9F, 0x60, 0x00, 0x10, 0x00, 0x03, 0x85, 0x60, 0x00, 0x10, 0x00, 0x0D, 0x6B, 0x60, 0x00,
   0x10, 0x00, 0x16, 0x84, 0x60, 0x00, 0x10, 0x00, 0x1E, 0x49, 0x60, 0x00, 0x10, 0x00, 0x1E, 0x49, 0x60, 0x00, 0x10, 0x00, 0x16, 0x84, 0x60, 0x00,
   0x10, 0x00, 0x0D, 0x6B, 0x60, 0x00, 0x10, 0x00, 0x03, 0x85, 0x60, 0x00, 0x10, 0x00, 0xFE, 0x9F, 0x60, 0x00, 0x10, 0x00, 0xFD, 0xD9, 0x60, 0x00,
   0x10, 0x00, 0xFE, 0x37, 0x60, 0x00, 0x10, 0x00, 0xFF, 0x93, 0x60, 0x00, 0xF0, 0x00, 0x10, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x10, 0x00, 0xF0, 0x00,
   0xF0, 0x00, 0x16, 0xE3, 0x60, 0x08, 0xF0, 0x00, 0x00, 0x64, 0x60, 0x00, 0xF0, 0x00, 0xFF, 0xA8, 0x60, 0x00, 0x10, 0x00, 0xFF, 0xA6, 0x60, 0x00,
   0x10, 0x00, 0xFF, 0xDF, 0x60, 0x00, 0x10, 0x00, 0x00, 0x01, 0x60, 0x00, 0x10, 0x00, 0x01, 0x28, 0x60, 0x00, 0x10, 0x00, 0x01, 0x2F, 0x60, 0x00,
   0x10, 0x00, 0xFD, 0x23, 0x60, 0x00, 0x10, 0x00, 0xFD, 0xA1, 0x60, 0x00, 0x10, 0x00, 0x03, 0x89, 0x60, 0x00, 0x10, 0x00, 0x02, 0x54, 0x60, 0x00,
   0x10, 0x00, 0x0E, 0xA2, 0x60, 0x00, 0x10, 0x00, 0x0C, 0x47, 0x60, 0x00, 0x10, 0x00, 0xF9, 0x42, 0x60, 0x00, 0x10, 0x00, 0xFB, 0xE1, 0x60, 0x00,
   0x10, 0x00, 0x00, 0x8C, 0x60, 0x00, 0x10, 0x00, 0xFE, 0x7D, 0x60, 0x00, 0x10, 0x00, 0x02, 0x54, 0x60, 0x00, 0x10, 0x00, 0x03, 0x89, 0x60, 0x00,
   0x10, 0x00, 0xFD, 0xA1, 0x60, 0x00, 0x10, 0x00, 0xFD, 0x23, 0x60, 0x00, 0x10, 0x00, 0x01, 0x2F, 0x60, 0x00, 0x10, 0x00, 0x01, 0x28, 0x60, 0x00,
   0x10, 0x00, 0x00, 0x01, 0x60, 0x00, 0x10, 0x00, 0xFF, 0xDF, 0x60, 0x00, 0x10, 0x00, 0xFF, 0xA6, 0x60, 0x00, 0x10, 0x00, 0xFF, 0xA8, 0x60, 0x00,
   0x10, 0x00, 0x00, 0x64, 0x60, 0x00, 0xF0, 0x00, 0x10, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x10, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x17, 0x0B, 0x60, 0x08,
   0xF0, 0x00, 0x00, 0x03, 0x60, 0x00, 0xF0, 0x00, 0x54, 0xC0, 0x60, 0x00, 0x10, 0x00, 0x00, 0x05, 0x60, 0x00, 0x10, 0x00, 0x00, 0x05, 0x60, 0x00,
   0x10, 0x00, 0x00, 0x0F, 0x60, 0x00, 0x10, 0x00, 0x00, 0x0F, 0x60, 0x00, 0x10, 0x00, 0x09, 0xC0, 0x60, 0x00, 0x10, 0x00, 0x0A, 0x20, 0x60, 0x00,
   0x10, 0x00, 0x1D, 0x40, 0x60, 0x00, 0x10, 0x00, 0x1E, 0x60, 0x60, 0x00, 0xF0, 0x00, 0x10, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x10, 0x00, 0xF0, 0x00,
   0xF0, 0x00, 0x70, 0x00, 0xD0, 0x08
};

unsigned char lutByteValues[]=
{
   /*
   0x40, 0x13, 0x41, 0x68, 0x41, 0xC1, 0x42, 0x14,
   0x47, 0xC5, 0x4D, 0x83, 0x4E, 0x49, 0x4E, 0x53,
   0x4F, 0x92, 0x4F, 0xEA, 0x50, 0x80, 0x56, 0x60,
   0x56, 0xD4, 0x56, 0xD9, 0x61, 0x9F, 0x61, 0xB6,
   0x64, 0x3B, 0x66, 0x09, 0x67, 0x0B, 0x67, 0x1A,
   0x68, 0x87, 0x68, 0xD4
   */
   0x80, 0x17, 0x80, 0x45, 0x80, 0x96, 0x81, 0x5B,
   0x82, 0xD6, 0x88, 0x0B, 0x88, 0x10, 0x88, 0xDB,
   0x89, 0x48, 0x8B, 0x0A, 0x8B, 0x12, 0x8B, 0x13,
   0x8B, 0x21, 0x8B, 0x26, 0x8C, 0x6F, 0x94, 0xD0,
   0x95, 0x20, 0x95, 0x33, 0x95, 0x6F, 0x95, 0xA8,
   0x95, 0xAC, 0x95, 0xC3, 0x95, 0xE1, 0x97, 0xC9,
   0x97, 0xF6, 0x98, 0x8D, 0x99, 0x00, 0x9A, 0x00,
   0x9E, 0x73, 0xA0, 0x12, 0xA1, 0x03, 0xA2, 0xF0,
   0xA3, 0x0D, 0xA3, 0x4C, 0xA3, 0x52, 0xA3, 0xED,
   0xA8, 0x31, 0xAB, 0x01, 0xAB, 0x1F, 0xAB, 0x4C,
   0xAC, 0x76, 0xAC, 0x88, 0xAD, 0xB0, 0xAD, 0xB7,
   0xB0, 0xE2, 0xB1, 0x01, 0xB1, 0x0B, 0xB1, 0x35,
   0xB1, 0x3B, 0xB1, 0x97, 0xB3, 0x03
};

const uint32_t patchSize   = sizeof(patchByteValues);
const uint8_t *pPatchBytes = &patchByteValues[0];

const uint32_t lutSize   = sizeof(lutByteValues);
const uint8_t *pLutBytes = &lutByteValues[0];

static const u8 init_para[] = {
//Set the Band related API settings...
11,   0x20, 0x0A, 0x01, 0x00, 0x01, 0x09, 0x38, 0x05, 0xDC, 0x05, 0xDC, //FM_BandWidth Auto,  FM_Set_Bandwidth (1, 1, 2360, 1500, 1200)
5,	   0x20, 0x14, 0x01, 0x00, 0x01,                                     //FM_MphSuppression,   FM_Set_MphSuppression (1, 1)
//5,    0x20, 0x16,	0x01,	0x00,	0x01,
7, 	0x20, 0x17, 0x01, 0x00, 0x01, 0x05, 0xDC,                         //FM_NoiseBlanker,   FM_Set_NoiseBlanker (1, 1, 1200)

//Set all Weaksignal API settings (LevelOffset)...
//Set the SoftMute API settings...
9,	   0x20, 0x2A, 0x01, 0x00, 0x03, 0x00, 0x64, 0x00, 0xFA,             //FM_SmlMode,   FM_Set_SoftMute_Level (1, 3, 100, 250)
11,	0x20, 0x28, 0x01, 0x00, 0x3C, 0x00, 0x78, 0x00, 0x0A, 0x00, 0x14, //FM_SmSlowAttack,FM_Set_SoftMute_Time (1, 60, 120, 10, 20)
9,	   0x20, 0x2C, 0x01, 0x00, 0x03, 0x01, 0x90, 0x03, 0xE8,             //FM_Smm,FM_Set_SoftMute_Mph (1, 3, 400, 1000)
9,	   0x20, 0x2B, 0x01, 0x00, 0x03, 0x01, 0x90, 0x03, 0xE8,             //FM_Smn,FM_Set_SoftMute_Noise (1, 3, 400, 1000)
7,	   0x20,	0x2D,	0x01,	0x00,	0x01,	0x00,	0x64,                         //FM_SmMaximum,FM_Set_SoftMute_Max (1, 1, 100)

//Set the HighCut API settings...
9,	   0x20,	0x34,	0x01,	0x00,	0x01,	0x01,	0xF4,	0x00,	0xC8,             //FM_HclMode,FM_Set_HighCut_Level (1, 1, 500, 200)
11,	0x20,	0x32,	0x01,	0x00,	0x3C,	0x00,	0x78,	0x00,	0x14,	0x00,	0x14, //FM_HcSlowAttack,FM_Set_HighCut_Time (1, 60, 120, 20, 20)
11,	0x20, 0x33,	0x01,	0x00,	0x01,	0x01,	0x90,	0x00,	0xC8,	0x03,	0x20, //FM_Hco,FM_Set_HighCut_Mod (1, 1, 400, 200, 800)
9,	   0x20,	0x36,	0x01,	0x00,	0x01,	0x00,	0x64,	0x00,	0x64,             //FM_Hcm,FM_Set_HighCut_Mph (1, 1, 100, 100)
9,	   0x20,	0x35,	0x01,	0x00,	0x01,	0x00,	0x64,	0x00,	0x64,             //FM_Hcn,FM_Set_HighCut_Noise (1, 1, 100, 100)
7,	   0x20,	0x38,	0x01,	0x00,	0x01,	0x3A,	0x98,                         //FM_HcMinimum,FM_Set_HighCut_Min (1, 1, 15000)
7,	   0x20,	0x3A,	0x01,	0x00,	0x01,	0x00,	0x0A,                         //FM_HcLowCutMinimum,FM_Set_LowCut_Min (1, 1, 100)
7,	   0x20,	0x37,	0x01,	0x00,	0x01,	0x05,	0xDC,                         //FM_HcMaximum,FM_Set_HighCut_Max (1, 1, 1500)

//Set the Stereo API settings...
9,	   0x20, 0x3E, 0x01, 0x00, 0x01, 0x01, 0xF4, 0x00, 0xFA,             //FM_StlMode,FM_Set_Stereo_Level (1, 1, 500, 250)
11,	0x20, 0x3C, 0x01, 0x00, 0x3C, 0x00, 0x78, 0x00, 0x0A, 0x00, 0x14, //FM_StSlowAttack,FM_Set_Stereo_Time (1, 60, 120, 10, 20)
11,	0x20, 0x3D, 0x01, 0x00, 0x01, 0x00, 0xC8, 0x00, 0xC8, 0x03, 0xE8, //FM_Sto,FM_Set_Stereo_Mod (1, 1, 200, 200, 1000)
9,	   0x20, 0x40, 0x01, 0x00, 0x01, 0x00, 0x64, 0x00, 0x64,             //FM_Stm,FM_Set_Stereo_Mph (1, 1, 100, 100)
9,	   0x20, 0x3F, 0x01, 0x00, 0x01, 0x00, 0x64, 0x00, 0x64,             //FM_Stn,FM_Set_Stereo_Noise (1, 1, 100, 100)
7,    0x20,	0x39,	0x01,	0x00,	0x01,	0x00,	0x50, 
9,    0x20,	0x4A,	0x01,	0x00,	0x03,	0x00,	0x50,	0x00,	0x80,
9,    0x20,	0x49,	0x01,	0x00,	0x03,	0x00,	0x50,	0x00,	0x80,

//Set_Deemphasis
5,    0x20,	0x1F,	0x01,	0x02,	0xEE,

//Set RDS
9,    0x20,	0x51,	0x01,	0x00,	0x01,	0x00,	0x02,	0x00,	0x02,

//Set the Audio and Application related API settings...
9,	   0x40, 0x03, 0x01, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, //AM_GPIO 0 Feature
9,	   0x40, 0x03, 0x01, 0x00, 0x01, 0x00, 0x21, 0x00, 0x00, //AM_GPIO 1 Feature
9,	   0x40, 0x03, 0x01, 0x00, 0x02, 0x00, 0x21, 0x00, 0x00, //AM_GPIO 2 Feature
9,	   0x40, 0x03, 0x01, 0x00, 0x00, 0x00, 0x20, 0x01, 0x01, //FM_GPIO 0 Feature set for RDS
9,	   0x40, 0x03, 0x01, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, //FM_GPIO 1 Feature
9,	   0x40, 0x03, 0x01, 0x00, 0x02, 0x00, 0x20, 0x00, 0x00, //FM_GPIO 2 Feature

5,    0x20, 0x1E, 0x01, 0x00, 0x01 ,
5,    0x20, 0x54, 0x01, 0x00, 0x00 ,
7,    0x20, 0x0B, 0x01, 0x03, 0x7A, 0x00, 0x00
//13,	0x30, 0x16, 0x01, 0x00, 0x21, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x11, 0x3A, //Dig_IO_IIS_SD_1 Mode
//5,	0x30, 0x0B, 0x01, 0x00, 0x00, //Mute
//5,	0x30, 0x0A, 0x01, 0xFF, 0xf0, //Volume
//7,	   0x30, 0x0D, 0x01, 0x00, 0x80, 0x00, 0xE0,//Audio Output Source DAC L/R
};

typedef enum
{
   eAR_TuningAction_Standby      = 0,
	eAR_TuningAction_Preset 	  = 1, /*!< Tune to new program with short mute time */
   eAR_TuningAction_Search       = 2, /*!< Tune to new program and stay muted */
   eAR_TuningAction_AF_Update    = 3, /*!< Tune to alternative frequency, store quality and tune back with inaudible mute */
   eAR_TuningAction_Jump         = 4, /*!< Tune to alternative frequency with short inaudible mute  */
   eAR_TuningAction_Check        = 5, /*!< Tune to alternative frequency and stay muted */
   eAR_TuningAction_End          = 7  /*!< Release the mute of a Search/Check action (frequency is ignored) */
} AR_TuningAction_t, *pAR_TuningAction_t;
#endif
