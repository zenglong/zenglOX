// zlox_audio.c -- something relate to audio

#include "zlox_monitor.h"
#include "zlox_task.h"
#include "zlox_kheap.h"
#include "zlox_sb16.h"
#include "zlox_audio.h"

ZLOX_SINT32 zlox_sb16_mixer_get_volume(ZLOX_AUD_CTRL_TYPE ctrl_type, 
					ZLOX_AUD_EXTRA_DATA * extra_data);
ZLOX_SINT32 zlox_sb16_mixer_set_volume(ZLOX_AUD_CTRL_TYPE ctrl_type, 
					ZLOX_AUD_EXTRA_DATA * extra_data);

extern ZLOX_BOOL dsp_start;
extern ZLOX_BOOL running;
extern ZLOX_UINT32 SB16_DspFillData;

ZLOX_UINT8 * audio_buf = ZLOX_NULL;
ZLOX_SINT32 audio_buf_size = 0;
ZLOX_SINT32 audio_buf_cpy_cnt = 0;
ZLOX_SINT32 audio_flip_flop = 0;
ZLOX_BOOL audio_end = ZLOX_FALSE;
ZLOX_BOOL audio_mixer_init_detected = ZLOX_FALSE;
ZLOX_BOOL audio_mixer_exist = ZLOX_FALSE;
ZLOX_BOOL audio_exist = ZLOX_FALSE;

ZLOX_SINT32 zlox_audio_init()
{
	if(zlox_sb16_init() == -1)
	{
		return -1;
	}
	return 0;
}

ZLOX_SINT32 zlox_audio_reset()
{
	if(zlox_sb16_dsp_reset() != 0)
	{
		audio_exist = ZLOX_FALSE;
		return -1;
	}
	audio_exist = ZLOX_TRUE;
	return 0;
}

// alloc resource before init
ZLOX_SINT32 zlox_audio_alloc_res_before_init()
{
	ZLOX_UINT32 DmaPhys;
	zlox_kmalloc_128k_align(ZLOX_SB_DMA_SIZE + 4096, &DmaPhys);
	if(DmaPhys == 0)
	{
		zlox_monitor_write("audio: DmaPhys alloc failed!\n");
		return -1;
	}
	return zlox_sb16_set_dma_phys(DmaPhys, ZLOX_TRUE);
}

ZLOX_SINT32 zlox_audio_set_databuf(ZLOX_UINT8 * userdata, ZLOX_SINT32 size)
{
	if(dsp_start)
		return -2;
	if(userdata == ZLOX_NULL)
		return -1;
	if(size <= 0)
		return -1;
	if(audio_buf != ZLOX_NULL)
	{
		zlox_kfree(audio_buf);
		audio_buf = ZLOX_NULL;
		audio_buf_size = 0;
	}
	audio_buf_size = size;
	audio_buf = (ZLOX_UINT8 *)zlox_kmalloc(audio_buf_size);
	if(audio_buf == ZLOX_NULL)
	{
		audio_buf_size = 0;
		return -1;
	}
	zlox_memcpy(audio_buf, userdata, audio_buf_size);
	return 0;
}

ZLOX_SINT32 zlox_audio_set_args(ZLOX_UINT32 bits, ZLOX_UINT32 sign, ZLOX_UINT32 stereo, ZLOX_UINT32 speed, 
			ZLOX_TASK * task)
{
	if(dsp_start)
		return -2;
	if(zlox_sb16_dsp_set_bits(bits) != 0)
		return -1;
	zlox_sb16_dsp_set_sign(sign);
	zlox_sb16_dsp_set_stereo(stereo);
	if(zlox_sb16_set_task(task) != 0)
		return -1;
	if(zlox_sb16_dsp_set_speed(speed, ZLOX_FALSE) != 0)
		return -1;
	return 0;
}

ZLOX_SINT32 zlox_audio_play()
{
	if(dsp_start)
		return -2;
	if(audio_buf == ZLOX_NULL || audio_buf_size == 0)
		return -1;
	ZLOX_UINT32 DmaPhys = zlox_sb16_get_dma_phys();
	audio_buf_cpy_cnt = 0;
	audio_flip_flop = 0;
	audio_end = ZLOX_FALSE;
	if(audio_buf_size < ZLOX_SB_DMA_SIZE)
	{
		zlox_memcpy((ZLOX_UINT8 *)DmaPhys, audio_buf, audio_buf_size);
		zlox_memset(((ZLOX_UINT8 *)DmaPhys + audio_buf_size), SB16_DspFillData, 
				(ZLOX_SB_DMA_SIZE - audio_buf_size));
		audio_buf_cpy_cnt += audio_buf_size;
	}
	else
	{
		zlox_memcpy((ZLOX_UINT8 *)DmaPhys, audio_buf, ZLOX_SB_DMA_SIZE);
		audio_buf_cpy_cnt += ZLOX_SB_DMA_SIZE;
	}
	zlox_sb16_dsp_start(DmaPhys, ZLOX_SB_WRITE_DMA);
	return 0;
}

ZLOX_SINT32 zlox_audio_pause()
{
	return zlox_sb16_dsp_pause();
}

ZLOX_SINT32 zlox_audio_continue()
{
	return zlox_sb16_dsp_continue();
}

ZLOX_SINT32 zlox_audio_exit()
{
	return zlox_sb16_dsp_exit();
}

ZLOX_SINT32 zlox_audio_mixer_init()
{
	if(!audio_mixer_init_detected)
	{
		if(zlox_sb16_mixer_init() == 0)
			audio_mixer_exist = ZLOX_TRUE;
		else
			audio_mixer_exist = ZLOX_FALSE;
		audio_mixer_init_detected = ZLOX_TRUE;
	}
	return (audio_mixer_exist ? 0 : -1);
}

ZLOX_SINT32 zlox_audio_mixer_get_set_volume(ZLOX_AUD_CTRL_TYPE ctrl_type, 
					ZLOX_AUD_EXTRA_DATA * extra_data)
{
	if(extra_data == ZLOX_NULL)
		return -1;
	switch(ctrl_type)
	{
	case ZLOX_ACT_MIXER_GET_VOLUME:
	case ZLOX_ACT_MIXER_GET_MASTER:
	case ZLOX_ACT_MIXER_GET_VOICE:
		return zlox_sb16_mixer_get_volume(ctrl_type, extra_data);
	case ZLOX_ACT_MIXER_SET_MASTER:
	case ZLOX_ACT_MIXER_SET_VOICE:
	case ZLOX_ACT_MIXER_SET_MIDI:
	case ZLOX_ACT_MIXER_SET_CD:
	case ZLOX_ACT_MIXER_SET_LINE:
	case ZLOX_ACT_MIXER_SET_MIC:
	case ZLOX_ACT_MIXER_SET_PC:
	case ZLOX_ACT_MIXER_SET_TREBLE:
	case ZLOX_ACT_MIXER_SET_BASS:
	case ZLOX_ACT_MIXER_SET_OUT_GAIN:
	case ZLOX_ACT_MIXER_SET_OUT_CTRL:
		return zlox_sb16_mixer_set_volume(ctrl_type, extra_data);
	default:
		break;
	}
	return -1;
}

ZLOX_VOID * zlox_audio_get_task()
{
	return zlox_sb16_get_task();
}

ZLOX_SINT32 zlox_audio_detect_aud_exist(ZLOX_AUD_EXTRA_DATA * extra_data)
{
	extra_data->audio.exist = audio_exist;
	return 0;
}

ZLOX_SINT32 zlox_audio_ctrl(ZLOX_AUD_CTRL_TYPE ctrl_type, 
				ZLOX_AUD_EXTRA_DATA * extra_data)
{
	ZLOX_UNUSED(extra_data);
	switch(ctrl_type)
	{
	case ZLOX_ACT_PAUSE:
		return zlox_audio_pause();
	case ZLOX_ACT_CONTINUE:
		return zlox_audio_continue();
	case ZLOX_ACT_EXIT:
		return zlox_audio_exit();
	case ZLOX_ACT_GET_PLAY_SIZE:
		return audio_buf_cpy_cnt;
	case ZLOX_ACT_DETECT_AUD_EXIST:
		return zlox_audio_detect_aud_exist(extra_data);
	case ZLOX_ACT_SET_FILL_DATA:
		SB16_DspFillData = extra_data->audio.FillData;
		return 0;
	case ZLOX_ACT_MIXER_INIT:
		return zlox_audio_mixer_init();
	case ZLOX_ACT_MIXER_GET_VOLUME:
	case ZLOX_ACT_MIXER_GET_MASTER:
	case ZLOX_ACT_MIXER_GET_VOICE:
	case ZLOX_ACT_MIXER_SET_MASTER:
	case ZLOX_ACT_MIXER_SET_VOICE:
	case ZLOX_ACT_MIXER_SET_MIDI:
	case ZLOX_ACT_MIXER_SET_CD:
	case ZLOX_ACT_MIXER_SET_LINE:
	case ZLOX_ACT_MIXER_SET_MIC:
	case ZLOX_ACT_MIXER_SET_PC:
	case ZLOX_ACT_MIXER_SET_TREBLE:
	case ZLOX_ACT_MIXER_SET_BASS:
	case ZLOX_ACT_MIXER_SET_OUT_GAIN:
	case ZLOX_ACT_MIXER_SET_OUT_CTRL:
		return zlox_audio_mixer_get_set_volume(ctrl_type, extra_data);
	default:
		break;
	}
	return -1;
}

ZLOX_BOOL zlox_audio_get_data()
{
	if(audio_buf == ZLOX_NULL || audio_buf_size == 0)
		return -1;
	ZLOX_UINT32 DmaPhys = zlox_sb16_get_dma_phys();
	if(audio_buf_cpy_cnt < audio_buf_size)
	{
		ZLOX_UINT8 * dest = (ZLOX_UINT8 *)(DmaPhys + audio_flip_flop * ZLOX_SB_DSP_FRAG_SIZE);
		ZLOX_UINT8 * src = audio_buf + audio_buf_cpy_cnt;
		ZLOX_SINT32 remain_size = audio_buf_size - audio_buf_cpy_cnt;
		if(remain_size < ZLOX_SB_DSP_FRAG_SIZE)
		{
			zlox_memcpy(dest, src, remain_size);
			zlox_memset(dest + remain_size, SB16_DspFillData, (ZLOX_SB_DSP_FRAG_SIZE - remain_size));
			audio_buf_cpy_cnt += remain_size;
		}
		else
		{
			zlox_memcpy(dest, src, ZLOX_SB_DSP_FRAG_SIZE);
			audio_buf_cpy_cnt += ZLOX_SB_DSP_FRAG_SIZE;
		}
		audio_flip_flop = (audio_flip_flop == 0 ? 1 : 0);
		if(audio_end != ZLOX_FALSE)
			audio_end = ZLOX_FALSE;
	}
	else
	{
		if(audio_end == ZLOX_TRUE)
			return ZLOX_FALSE;
		ZLOX_UINT8 * dest = (ZLOX_UINT8 *)(DmaPhys + audio_flip_flop * ZLOX_SB_DSP_FRAG_SIZE);
		zlox_memset(dest, SB16_DspFillData, ZLOX_SB_DSP_FRAG_SIZE);
		audio_end = ZLOX_TRUE;
	}
	return ZLOX_TRUE;
}

ZLOX_SINT32 zlox_audio_end()
{
	if(audio_buf != ZLOX_NULL)
	{
		zlox_kfree(audio_buf);
		audio_buf = ZLOX_NULL;
	}
	audio_buf_size = 0;
	audio_buf_cpy_cnt = 0;
	audio_flip_flop = 0;
	audio_end = ZLOX_FALSE;
	return 0;
}

