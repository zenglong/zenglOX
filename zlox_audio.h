/*zlox_audio.h the audio header*/

#ifndef _ZLOX_AUDIO_H_
#define _ZLOX_AUDIO_H_

#include "zlox_common.h"

typedef enum _ZLOX_AUD_CTRL_TYPE
{
	ZLOX_ACT_PAUSE,
	ZLOX_ACT_CONTINUE,
	ZLOX_ACT_EXIT,
	ZLOX_ACT_GET_PLAY_SIZE,
	ZLOX_ACT_DETECT_AUD_EXIST,
	ZLOX_ACT_SET_FILL_DATA,
	ZLOX_ACT_MIXER_INIT,
	ZLOX_ACT_MIXER_GET_VOLUME,
	ZLOX_ACT_MIXER_GET_MASTER,
	ZLOX_ACT_MIXER_GET_VOICE,
	ZLOX_ACT_MIXER_SET_MASTER,
	ZLOX_ACT_MIXER_SET_VOICE,
	ZLOX_ACT_MIXER_SET_MIDI,
	ZLOX_ACT_MIXER_SET_CD,
	ZLOX_ACT_MIXER_SET_LINE,
	ZLOX_ACT_MIXER_SET_MIC,
	ZLOX_ACT_MIXER_SET_PC,
	ZLOX_ACT_MIXER_SET_TREBLE,
	ZLOX_ACT_MIXER_SET_BASS,
	ZLOX_ACT_MIXER_SET_OUT_GAIN,
	ZLOX_ACT_MIXER_SET_OUT_CTRL,
}ZLOX_AUD_CTRL_TYPE;

typedef struct _ZLOX_AUD_MIXER
{
	ZLOX_SINT32 master_left;
	ZLOX_SINT32 master_right;
	ZLOX_SINT32 voice_left;
	ZLOX_SINT32 voice_right;
	ZLOX_SINT32 midi_left;
	ZLOX_SINT32 midi_right;
	ZLOX_SINT32 cd_left;
	ZLOX_SINT32 cd_right;
	ZLOX_SINT32 line_left;
	ZLOX_SINT32 line_right;
	ZLOX_SINT32 mic_volume;
	ZLOX_SINT32 pc_volume;
	ZLOX_SINT32 treble_left;
	ZLOX_SINT32 treble_right;
	ZLOX_SINT32 bass_left;
	ZLOX_SINT32 bass_right;
	ZLOX_SINT32 output_gain_left;
	ZLOX_SINT32 output_gain_right;
	ZLOX_SINT32 output_ctrl;
}ZLOX_AUD_MIXER;

typedef struct _ZLOX_AUD_AUDIO
{
	ZLOX_UINT32 FillData;
	ZLOX_BOOL exist;
}ZLOX_AUD_AUDIO;

typedef struct _ZLOX_AUD_EXTRA_DATA
{
	ZLOX_AUD_MIXER mixer;
	ZLOX_AUD_AUDIO audio;
}ZLOX_AUD_EXTRA_DATA;

ZLOX_SINT32 zlox_audio_init();
ZLOX_SINT32 zlox_audio_reset();
// alloc resource before init
ZLOX_SINT32 zlox_audio_alloc_res_before_init();
ZLOX_SINT32 zlox_audio_set_databuf(ZLOX_UINT8 * userdata, ZLOX_SINT32 size);
ZLOX_SINT32 zlox_audio_set_args(ZLOX_UINT32 bits, ZLOX_UINT32 sign, ZLOX_UINT32 stereo, ZLOX_UINT32 speed, 
			ZLOX_TASK * task);
ZLOX_SINT32 zlox_audio_play();
ZLOX_VOID * zlox_audio_get_task();
ZLOX_SINT32 zlox_audio_ctrl(ZLOX_AUD_CTRL_TYPE ctrl_type, 
			ZLOX_AUD_EXTRA_DATA * extra_data);
ZLOX_BOOL zlox_audio_get_data();
ZLOX_SINT32 zlox_audio_end();

#endif // _ZLOX_AUDIO_H_

