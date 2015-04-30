// audio.h Defines the structures and prototypes needed to audio.

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include "common.h"

typedef enum _AUD_CTRL_TYPE
{
	ACT_PAUSE,
	ACT_CONTINUE,
	ACT_EXIT,
	ACT_GET_PLAY_SIZE,
	ACT_DETECT_AUD_EXIST,
	ACT_SET_FILL_DATA,
	ACT_MIXER_INIT,
	ACT_MIXER_GET_VOLUME,
	ACT_MIXER_GET_MASTER,
	ACT_MIXER_GET_VOICE,
	ACT_MIXER_SET_MASTER,
	ACT_MIXER_SET_VOICE,
	ACT_MIXER_SET_MIDI,
	ACT_MIXER_SET_CD,
	ACT_MIXER_SET_LINE,
	ACT_MIXER_SET_MIC,
	ACT_MIXER_SET_PC,
	ACT_MIXER_SET_TREBLE,
	ACT_MIXER_SET_BASS,
	ACT_MIXER_SET_OUT_GAIN,
	ACT_MIXER_SET_OUT_CTRL,
}AUD_CTRL_TYPE;

typedef struct _AUD_MIXER
{
	SINT32 master_left;
	SINT32 master_right;
	SINT32 voice_left;
	SINT32 voice_right;
	SINT32 midi_left;
	SINT32 midi_right;
	SINT32 cd_left;
	SINT32 cd_right;
	SINT32 line_left;
	SINT32 line_right;
	SINT32 mic_volume;
	SINT32 pc_volume;
	SINT32 treble_left;
	SINT32 treble_right;
	SINT32 bass_left;
	SINT32 bass_right;
	SINT32 output_gain_left;
	SINT32 output_gain_right;
	SINT32 output_ctrl;
}AUD_MIXER;

typedef struct _AUD_AUDIO
{
	UINT32 FillData;
	BOOL exist;
}AUD_AUDIO;

typedef struct _AUD_EXTRA_DATA
{
	AUD_MIXER mixer;
	AUD_AUDIO audio;
}AUD_EXTRA_DATA;

#endif // _AUDIO_H_

