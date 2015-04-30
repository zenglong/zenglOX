// zlox_sb16.c -- something relate to Sound Blaster 16 

#include "zlox_monitor.h"
#include "zlox_isr.h"
#include "zlox_task.h"
#include "zlox_sb16.h"
#include "zlox_audio.h"

ZLOX_UINT32 SB16_DmaPhys = 0;
ZLOX_TASK * SB16_Task = ZLOX_NULL;
ZLOX_SINT32 DspVersion[2];
ZLOX_UINT32 SB16_DspFillData = ZLOX_SB_DEFAULT_FILL_DATA;

static ZLOX_UINT32 DspStereo = ZLOX_SB_DEFAULT_STEREO;
static ZLOX_UINT32 DspSpeed = ZLOX_SB_DEFAULT_SPEED;
static ZLOX_UINT32 DspBits = ZLOX_SB_DEFAULT_BITS;
static ZLOX_UINT32 DspSign = ZLOX_SB_DEFAULT_SIGN;

ZLOX_BOOL dsp_start = ZLOX_FALSE;
ZLOX_BOOL running = ZLOX_FALSE;
ZLOX_BOOL has_vmware_cont = ZLOX_FALSE;

extern ZLOX_ISR_CALLBACK interrupt_callbacks[256];

static ZLOX_VOID zlox_sb16_callback();
ZLOX_SINT32 zlox_sb16_dsp_reenable_int();
ZLOX_SINT32 zlox_sb16_dsp_start(ZLOX_UINT32 DmaPhys, ZLOX_SINT32 DmaMode);
ZLOX_SINT32 zlox_sb16_dsp_dma_setup(ZLOX_UINT32 DmaPhys, ZLOX_SINT32 count, ZLOX_SINT32 DmaMode);
ZLOX_SINT32 zlox_sb16_dsp_set_speed(ZLOX_UINT32 speed, ZLOX_BOOL need_cmd);
ZLOX_SINT32 zlox_sb16_dsp_reset();
ZLOX_SINT32 zlox_sb16_dsp_command(ZLOX_SINT32 value);
ZLOX_SINT32 zlox_sb16_mixer_set(ZLOX_SINT32 reg, ZLOX_SINT32 data);
ZLOX_SINT32 zlox_sb16_mixer_get(ZLOX_SINT32 reg);

ZLOX_SINT32 zlox_sb16_init()
{
	ZLOX_SINT32 i;
	//ZLOX_SINT32 DspVersion[2];
	if(zlox_sb16_dsp_reset() != 0) { 
		zlox_monitor_write("No SoundBlaster!\n"); /* No SoundBlaster */
		return -1;
	}
	zlox_monitor_write("SoundBlaster is detected! ");
	DspVersion[0] = DspVersion[1] = 0;
	zlox_sb16_dsp_command(ZLOX_SB_DSP_GET_VERSION);

	for(i = 1000; i; i--) {
		if(zlox_inb(ZLOX_SB_DSP_DATA_AVL) & 0x80) {		
			if(DspVersion[0] == 0) {
				DspVersion[0] = zlox_inb(ZLOX_SB_DSP_READ);
			} else {
				DspVersion[1] = zlox_inb(ZLOX_SB_DSP_READ);
				break;
			}
		}
	}

	if(DspVersion[0] < 4) {
		zlox_monitor_write("sb16: No SoundBlaster 16 compatible card detected\n");
		return -1;
	} 

	zlox_monitor_write("sb16: SoundBlaster DSP version ");
	zlox_monitor_write_dec(DspVersion[0]);
	zlox_monitor_write(".");
	zlox_monitor_write_dec(DspVersion[1]);
	zlox_monitor_write(" detected!\n");

	/* set SB to use our IRQ and DMA channels */
	zlox_sb16_mixer_set(ZLOX_SB_MIXER_SET_IRQ, (1 << (ZLOX_SB_IRQ / 2 - 1)));
	zlox_sb16_mixer_set(ZLOX_SB_MIXER_SET_DMA, (1 << ZLOX_SB_DMA_8 | 1 << ZLOX_SB_DMA_16));

	if(interrupt_callbacks[ZLOX_IRQ0 + ZLOX_SB_IRQ] != 0)
	{
		zlox_monitor_write("sb16: irq has been occupied!\n");
		return -1;
	}

	zlox_register_interrupt_callback(ZLOX_IRQ0 + ZLOX_SB_IRQ, 
				&zlox_sb16_callback);
	return 0;
}

static ZLOX_VOID zlox_sb16_callback()
{
	//zlox_monitor_write("sb16: in callback\n");
	if(running) 
	{
		ZLOX_TASK_MSG msg = {0};
		ZLOX_SINT32 status = zlox_sb16_mixer_get(ZLOX_SB_MIXER_IRQ_STATUS);
		/*zlox_monitor_write("[sb16: irq: \n");
		zlox_monitor_write_hex(status);
		zlox_monitor_write("]");*/
		if(!(status & 0x03)) // must be 8 bit or 16 bit irq interrupt!
			return;
		if(zlox_audio_get_data() == ZLOX_TRUE)
		{
			if(SB16_Task != ZLOX_NULL)
			{
				msg.type = ZLOX_MT_AUDIO_INT;
				zlox_send_tskmsg(SB16_Task,&msg);
			}
			if(has_vmware_cont)
			{
				zlox_sb16_dsp_command((DspBits == 8 ? ZLOX_SB_DSP_CMD_DMA8CONT : 
						ZLOX_SB_DSP_CMD_DMA16CONT));
			}
			zlox_sb16_dsp_reenable_int();
			return;
		}
		else
		{
			zlox_sb16_dsp_command((DspBits == 8 ? ZLOX_SB_DSP_CMD_DMA8EXIT : 
							ZLOX_SB_DSP_CMD_DMA16EXIT));
			dsp_start = ZLOX_FALSE;
			running = ZLOX_FALSE;
			zlox_audio_end();
			if(SB16_Task != ZLOX_NULL)
			{
				msg.type = ZLOX_MT_AUDIO_END;
				zlox_send_tskmsg(SB16_Task,&msg);
				SB16_Task = ZLOX_NULL;
			}
			if(has_vmware_cont)
				has_vmware_cont = ZLOX_FALSE;
			zlox_sb16_dsp_reenable_int();
			return;
		}
	}
}

ZLOX_SINT32 zlox_sb16_dsp_reenable_int()
{
	zlox_inb((DspBits == 8 ? ZLOX_SB_DSP_DATA_AVL : ZLOX_SB_DSP_DATA16_AVL));
	return 0;
}

ZLOX_SINT32 zlox_sb16_dsp_pause()
{
	if(running)
	{
		zlox_sb16_dsp_command((DspBits == 8 ? ZLOX_SB_DSP_CMD_DMA8PAUSE : 
				ZLOX_SB_DSP_CMD_DMA16PAUSE));
		running = ZLOX_FALSE;
		zlox_sb16_dsp_reenable_int();
		return 0;
	}
	return -1;
}

ZLOX_SINT32 zlox_sb16_dsp_continue()
{
	if(dsp_start && !running)
	{
		zlox_sb16_dsp_command((DspBits == 8 ? ZLOX_SB_DSP_CMD_DMA8CONT : 
				ZLOX_SB_DSP_CMD_DMA16CONT));
		running = ZLOX_TRUE;
		if(DspVersion[1] == 13)
			has_vmware_cont = ZLOX_TRUE;
		return 0;
	}
	return -1;
}

ZLOX_SINT32 zlox_sb16_dsp_exit()
{
	if(!dsp_start)
		return -1;
	zlox_sb16_dsp_command((DspBits == 8 ? ZLOX_SB_DSP_CMD_DMA8EXIT : 
					ZLOX_SB_DSP_CMD_DMA16EXIT));
	dsp_start = ZLOX_FALSE;
	running = ZLOX_FALSE;
	zlox_audio_end();
	if(SB16_Task != ZLOX_NULL)
		SB16_Task = ZLOX_NULL;
	if(has_vmware_cont)
		has_vmware_cont = ZLOX_FALSE;
	zlox_sb16_dsp_reenable_int();
	return 0;
}

ZLOX_SINT32 zlox_sb16_dsp_start(ZLOX_UINT32 DmaPhys, ZLOX_SINT32 DmaMode)
{
	if(dsp_start)
		return -2;
	if(DmaPhys == 0)
		DmaPhys = SB16_DmaPhys;
	zlox_sb16_dsp_reset();
	zlox_sb16_dsp_dma_setup(DmaPhys, ZLOX_SB_DMA_SIZE, DmaMode);
	zlox_sb16_dsp_set_speed(DspSpeed, ZLOX_TRUE);
	/* Put the speaker on */
	if(DmaMode == ZLOX_SB_WRITE_DMA)
	{
		zlox_sb16_dsp_command(ZLOX_SB_DSP_CMD_SPKON); /* put speaker on */
		/* Program DSP with dma mode */
		zlox_sb16_dsp_command((DspBits == 8 ? ZLOX_SB_DSP_CMD_8BITAUTO_OUT : 
						ZLOX_SB_DSP_CMD_16BITAUTO_OUT)); 
	} else {
		zlox_sb16_dsp_command(ZLOX_SB_DSP_CMD_SPKOFF); /* put speaker off */
		/* Program DSP with dma mode */
		zlox_sb16_dsp_command((DspBits == 8 ? ZLOX_SB_DSP_CMD_8BITAUTO_IN : 
						ZLOX_SB_DSP_CMD_16BITAUTO_IN));
	}
	/* Program DSP with transfer mode */
	if (!DspSign) 
	{
		zlox_sb16_dsp_command((DspStereo == 1 ? 
				ZLOX_SB_DSP_MODE_STEREO_US : ZLOX_SB_DSP_MODE_MONO_US));
	}
	else 
	{
		zlox_sb16_dsp_command((DspStereo == 1 ? 
				ZLOX_SB_DSP_MODE_STEREO_S : ZLOX_SB_DSP_MODE_MONO_S));
	}
	if (DspBits == 8) 
	{	/* 8 bit transfer */
		/* #bytes - 1 */
		zlox_sb16_dsp_command((ZLOX_SB_DSP_FRAG_SIZE - 1) >> 0); 
		zlox_sb16_dsp_command((ZLOX_SB_DSP_FRAG_SIZE - 1) >> 8);
	}
	else {
		/* 16 bit transfer */
		/* #words - 1 */
		zlox_sb16_dsp_command((ZLOX_SB_DSP_FRAG_SIZE - 2) >> 1);
		zlox_sb16_dsp_command((ZLOX_SB_DSP_FRAG_SIZE - 2) >> 9);
	}

	dsp_start = ZLOX_TRUE;
	running = ZLOX_TRUE;
	return 0;
}

ZLOX_SINT32 zlox_sb16_dsp_dma_setup(ZLOX_UINT32 address, ZLOX_SINT32 count, ZLOX_SINT32 DmaMode)
{
	if(DspBits == 8) 
	{   /* 8 bit sound */
		count--;
		zlox_outb(ZLOX_SB_DMA8_MASK, ZLOX_SB_DMA_8 | 0x04); /* Disable DMA channel */
		zlox_outb(ZLOX_SB_DMA8_CLEAR, 0x00); /* Clear flip flop */
		zlox_outb(ZLOX_SB_DMA8_MODE, (DmaMode == ZLOX_SB_WRITE_DMA ? 
						ZLOX_SB_DMA8_AUTO_PLAY : ZLOX_SB_DMA8_AUTO_REC));
		zlox_outb(ZLOX_SB_DMA8_ADDR, (ZLOX_UINT8)(address >>  0)); /* Low_byte of address */
		zlox_outb(ZLOX_SB_DMA8_ADDR, (ZLOX_UINT8)(address >>  8)); /* High byte of address */
		zlox_outb(ZLOX_SB_DMA8_PAGE, (ZLOX_UINT8)(address >> 16)); /* 64K page number */
		zlox_outb(ZLOX_SB_DMA8_COUNT, (ZLOX_UINT8)(count >> 0)); /* Low byte of count */
		zlox_outb(ZLOX_SB_DMA8_COUNT, (ZLOX_UINT8)(count >> 8)); /* High byte of count */
		zlox_outb(ZLOX_SB_DMA8_MASK, ZLOX_SB_DMA_8); /* Enable DMA channel */
	}
	else {  /* 16 bit sound */
		count -= 2;
		zlox_outb(ZLOX_SB_DMA16_MASK, (ZLOX_SB_DMA_16 & 3) | 0x04); /* Disable DMA channel */
		zlox_outb(ZLOX_SB_DMA16_CLEAR, 0x00); /* Clear flip flop */
		/* Set dma mode */
		zlox_outb(ZLOX_SB_DMA16_MODE, (DmaMode == ZLOX_SB_WRITE_DMA ? 
						ZLOX_SB_DMA16_AUTO_PLAY : ZLOX_SB_DMA16_AUTO_REC));
		zlox_outb(ZLOX_SB_DMA16_ADDR, (ZLOX_UINT8)((address >> 1) & 0xFF));  /* Low_byte of address */
		zlox_outb(ZLOX_SB_DMA16_ADDR, (ZLOX_UINT8)((address >> 9) & 0x7F));  /* High byte of address */
		zlox_outb(ZLOX_SB_DMA16_PAGE, (ZLOX_UINT8)((address >> 16) & 0xFE)); /* 128K page number */
		zlox_outb(ZLOX_SB_DMA16_COUNT, (ZLOX_UINT8)(count >> 1));    /* Low byte of count */
		zlox_outb(ZLOX_SB_DMA16_COUNT, (ZLOX_UINT8)(count >> 9));    /* High byte of count */
		zlox_outb(ZLOX_SB_DMA16_MASK, (ZLOX_SB_DMA_16 & 3)); /* Enable DMA channel */
	}
	return 0;
}

ZLOX_SINT32 zlox_sb16_dsp_set_speed(ZLOX_UINT32 speed, ZLOX_BOOL need_cmd)
{
	if(speed < ZLOX_SB_DSP_MIN_SPEED || speed > ZLOX_SB_DSP_MAX_SPEED) {
		return -1;
	}

	if(need_cmd)
	{
		/* Soundblaster 16 can be programmed with real sample rates
		* instead of time constants
		*
		* Since you cannot sample and play at the same time
		* we set in- and output rate to the same value 
		*/

		zlox_sb16_dsp_command(ZLOX_SB_DSP_INPUT_RATE); /* set input rate */
		zlox_sb16_dsp_command(speed >> 8); /* high byte of speed */
		zlox_sb16_dsp_command(speed); /* low byte of speed */
		zlox_sb16_dsp_command(ZLOX_SB_DSP_OUTPUT_RATE); /* same for output rate */
		zlox_sb16_dsp_command(speed >> 8);
		zlox_sb16_dsp_command(speed);
	}

	DspSpeed = speed;

	return 0;
}

ZLOX_SINT32 zlox_sb16_dsp_reset()
{
	ZLOX_SINT32 i;
	zlox_outb(ZLOX_SB_DSP_RESET, 1);
	for(i = 0; i < 1000; i++)
		; /* wait a while */
	zlox_outb(ZLOX_SB_DSP_RESET, 0);

	for(i = 0; i < 1000 && 
		!(zlox_inb(ZLOX_SB_DSP_DATA_AVL) & 0x80); 
		i++)
		;

	if(zlox_inb(ZLOX_SB_DSP_READ) != 0xAA) 
		return -1;
	return 0;
}

ZLOX_SINT32 zlox_sb16_dsp_command(ZLOX_SINT32 value)
{
	ZLOX_SINT32 i;

	for (i = 0; i < ZLOX_SB_TIMEOUT; i++) {
		if((zlox_inb(ZLOX_SB_DSP_STATUS) & 0x80) == 0) {
			zlox_outb(ZLOX_SB_DSP_COMMAND, value);
			return 0;
		}
	}

	zlox_monitor_write("sb16: SoundBlaster: DSP Command(");
	zlox_monitor_write_hex(value);
	zlox_monitor_write(") timeout\n");
	return -1;
}

ZLOX_SINT32 zlox_sb16_dsp_set_bits(ZLOX_UINT32 bits)
{
	/* Sanity checks */
	if(bits != 8 && bits != 16) {
		return -1;
	}

	if(dsp_start)
		return -2;
	DspBits = bits; 

	return 0;
}

ZLOX_SINT32 zlox_sb16_dsp_set_sign(ZLOX_UINT32 sign)
{
	DspSign = (sign > 0 ? 1 : 0); 

	return 0;
}

ZLOX_SINT32 zlox_sb16_dsp_set_stereo(ZLOX_UINT32 stereo)
{
	if(stereo) { 
		DspStereo = 1;
	} else {
		DspStereo = 0;
	}

	return 0;
}

ZLOX_SINT32 zlox_sb16_set_dma_phys(ZLOX_UINT32 DmaPhys, ZLOX_BOOL need_clear)
{
	if(DmaPhys == 0)
	{
		zlox_monitor_write("sb16: DmaPhys is zero!\n");
		return -1;
	}
	SB16_DmaPhys = DmaPhys;
	if(need_clear)
		zlox_memset((ZLOX_UINT8 *)(DmaPhys), SB16_DspFillData, ZLOX_SB_DMA_SIZE);
	return 0;
}

ZLOX_UINT32 zlox_sb16_get_dma_phys()
{
	return SB16_DmaPhys;
}

ZLOX_SINT32 zlox_sb16_set_task(ZLOX_TASK * task)
{
	if(task == ZLOX_NULL)
		return -1;
	SB16_Task = task;
	return 0;
}

ZLOX_VOID * zlox_sb16_get_task()
{
	return (ZLOX_VOID *)SB16_Task;
}

/*=========================================================================*
 *				mixer				   
 *=========================================================================*/
ZLOX_SINT32 zlox_sb16_mixer_init()
{
	/* Try to detect the mixer by writing to MIXER_DAC_LEVEL if the
	* value written can be read back the mixer is there
	*/

	zlox_sb16_mixer_set(ZLOX_SB_MIXER_DAC_LEVEL, 0x10);  /* write something to it */
	if(zlox_sb16_mixer_get(ZLOX_SB_MIXER_DAC_LEVEL) != 0x10) {
		return -1;
	}

	/* Enable Automatic Gain Control */
	zlox_sb16_mixer_set(ZLOX_SB_MIXER_AGC, 0x01);

	return 0;
}

ZLOX_SINT32 zlox_sb16_mixer_get_volume(ZLOX_AUD_CTRL_TYPE ctrl_type, 
					ZLOX_AUD_EXTRA_DATA * extra_data)
{
	ZLOX_SINT32 shift = 3;
	if(ctrl_type == ZLOX_ACT_MIXER_GET_VOLUME)
	{
		extra_data->mixer.master_left = zlox_sb16_mixer_get(ZLOX_SB_MIXER_MASTER_LEFT);
		extra_data->mixer.master_right = zlox_sb16_mixer_get(ZLOX_SB_MIXER_MASTER_RIGHT);
		extra_data->mixer.master_left >>= shift;
		extra_data->mixer.master_right >>= shift;
		extra_data->mixer.voice_left = zlox_sb16_mixer_get(ZLOX_SB_MIXER_DAC_LEFT);
		extra_data->mixer.voice_right = zlox_sb16_mixer_get(ZLOX_SB_MIXER_DAC_RIGHT);
		extra_data->mixer.voice_left >>= shift;
		extra_data->mixer.voice_right >>= shift;
		extra_data->mixer.midi_left = zlox_sb16_mixer_get(ZLOX_SB_MIXER_FM_LEFT);
		extra_data->mixer.midi_right = zlox_sb16_mixer_get(ZLOX_SB_MIXER_FM_RIGHT);
		extra_data->mixer.midi_left >>= shift;
		extra_data->mixer.midi_right >>= shift;
		extra_data->mixer.cd_left = zlox_sb16_mixer_get(ZLOX_SB_MIXER_CD_LEFT);
		extra_data->mixer.cd_right = zlox_sb16_mixer_get(ZLOX_SB_MIXER_CD_RIGHT);
		extra_data->mixer.cd_left >>= shift;
		extra_data->mixer.cd_right >>= shift;
		extra_data->mixer.line_left = zlox_sb16_mixer_get(ZLOX_SB_MIXER_LINE_LEFT);
		extra_data->mixer.line_right = zlox_sb16_mixer_get(ZLOX_SB_MIXER_LINE_RIGHT);
		extra_data->mixer.line_left >>= shift;
		extra_data->mixer.line_right >>= shift;
		extra_data->mixer.mic_volume = zlox_sb16_mixer_get(ZLOX_SB_MIXER_MIC_LEVEL);
		extra_data->mixer.mic_volume >>= shift;
		extra_data->mixer.pc_volume = zlox_sb16_mixer_get(ZLOX_SB_MIXER_PC_LEVEL);
		extra_data->mixer.pc_volume >>= 6;
		extra_data->mixer.treble_left = zlox_sb16_mixer_get(ZLOX_SB_MIXER_TREBLE_LEFT);
		extra_data->mixer.treble_right = zlox_sb16_mixer_get(ZLOX_SB_MIXER_TREBLE_RIGHT);
		extra_data->mixer.treble_left >>= 4;
		extra_data->mixer.treble_right >>= 4;
		extra_data->mixer.bass_left = zlox_sb16_mixer_get(ZLOX_SB_MIXER_BASS_LEFT);
		extra_data->mixer.bass_right = zlox_sb16_mixer_get(ZLOX_SB_MIXER_BASS_RIGHT);
		extra_data->mixer.bass_left >>= 4;
		extra_data->mixer.bass_right >>= 4;
		extra_data->mixer.output_gain_left = zlox_sb16_mixer_get(ZLOX_SB_MIXER_GAIN_OUT_LEFT);
		extra_data->mixer.output_gain_right = zlox_sb16_mixer_get(ZLOX_SB_MIXER_GAIN_OUT_RIGHT);
		extra_data->mixer.output_gain_left >>= 6;
		extra_data->mixer.output_gain_right >>= 6;
		extra_data->mixer.output_ctrl = zlox_sb16_mixer_get(ZLOX_SB_MIXER_OUTPUT_CTRL);
		return 0;
	}
	else if(ctrl_type == ZLOX_ACT_MIXER_GET_MASTER)
	{
		extra_data->mixer.master_left = zlox_sb16_mixer_get(ZLOX_SB_MIXER_MASTER_LEFT);
		extra_data->mixer.master_right = zlox_sb16_mixer_get(ZLOX_SB_MIXER_MASTER_RIGHT);
		extra_data->mixer.master_left >>= shift;
		extra_data->mixer.master_right >>= shift;
		return 0;
	}
	else if(ctrl_type == ZLOX_ACT_MIXER_GET_VOICE)
	{
		extra_data->mixer.voice_left = zlox_sb16_mixer_get(ZLOX_SB_MIXER_DAC_LEFT);
		extra_data->mixer.voice_right = zlox_sb16_mixer_get(ZLOX_SB_MIXER_DAC_RIGHT);
		extra_data->mixer.voice_left >>= shift;
		extra_data->mixer.voice_right >>= shift;
		return 0;
	}
	return -1;
}

ZLOX_SINT32 zlox_sb16_mixer_set_volume(ZLOX_AUD_CTRL_TYPE ctrl_type, 
					ZLOX_AUD_EXTRA_DATA * extra_data)
{
	ZLOX_SINT32 shift = 3;
	ZLOX_SINT32 max_level = 0x1F;
	ZLOX_SINT32 reg_left, reg_right;
	ZLOX_SINT32 level_left, level_right;
	switch(ctrl_type)
	{
	case ZLOX_ACT_MIXER_SET_MASTER:
		reg_left = ZLOX_SB_MIXER_MASTER_LEFT;
		reg_right = ZLOX_SB_MIXER_MASTER_RIGHT;
		level_left = extra_data->mixer.master_left;
		level_right = extra_data->mixer.master_right;
		break;
	case ZLOX_ACT_MIXER_SET_VOICE:
		reg_left = ZLOX_SB_MIXER_DAC_LEFT;
		reg_right = ZLOX_SB_MIXER_DAC_RIGHT;
		level_left = extra_data->mixer.voice_left;
		level_right = extra_data->mixer.voice_right;
		break;
	case ZLOX_ACT_MIXER_SET_MIDI:
		reg_left = ZLOX_SB_MIXER_FM_LEFT;
		reg_right = ZLOX_SB_MIXER_FM_RIGHT;
		level_left = extra_data->mixer.midi_left;
		level_right = extra_data->mixer.midi_right;
		break;
	case ZLOX_ACT_MIXER_SET_CD:
		reg_left = ZLOX_SB_MIXER_CD_LEFT;
		reg_right = ZLOX_SB_MIXER_CD_RIGHT;
		level_left = extra_data->mixer.cd_left;
		level_right = extra_data->mixer.cd_right;
		break;
	case ZLOX_ACT_MIXER_SET_LINE:
		reg_left = ZLOX_SB_MIXER_LINE_LEFT;
		reg_right = ZLOX_SB_MIXER_LINE_RIGHT;
		level_left = extra_data->mixer.line_left;
		level_right = extra_data->mixer.line_right;
		break;
	case ZLOX_ACT_MIXER_SET_MIC:
		reg_right = reg_left = ZLOX_SB_MIXER_MIC_LEVEL;
		level_right = level_left = extra_data->mixer.mic_volume;
		break;
	case ZLOX_ACT_MIXER_SET_PC:
		reg_right = reg_left = ZLOX_SB_MIXER_PC_LEVEL;
		level_right = level_left = extra_data->mixer.pc_volume;
		shift = 6;
		max_level = 0x3;
		break;
	case ZLOX_ACT_MIXER_SET_TREBLE:
		reg_left = ZLOX_SB_MIXER_TREBLE_LEFT;
		reg_right = ZLOX_SB_MIXER_TREBLE_RIGHT;
		level_left = extra_data->mixer.treble_left;
		level_right = extra_data->mixer.treble_right;
		shift = 4;
		max_level = 0xF;
		break;
	case ZLOX_ACT_MIXER_SET_BASS:
		reg_left = ZLOX_SB_MIXER_BASS_LEFT;
		reg_right = ZLOX_SB_MIXER_BASS_RIGHT;
		level_left = extra_data->mixer.bass_left;
		level_right = extra_data->mixer.bass_right;
		shift = 4;
		max_level = 0xF;
		break;
	case ZLOX_ACT_MIXER_SET_OUT_GAIN:
		reg_left = ZLOX_SB_MIXER_GAIN_OUT_LEFT;
		reg_right = ZLOX_SB_MIXER_GAIN_OUT_RIGHT;
		level_left = extra_data->mixer.output_gain_left;
		level_right = extra_data->mixer.output_gain_right;
		shift = 6;
		max_level = 0x3;
		break;
	case ZLOX_ACT_MIXER_SET_OUT_CTRL:
		reg_right = reg_left = ZLOX_SB_MIXER_OUTPUT_CTRL;
		level_right = level_left = extra_data->mixer.output_ctrl;
		shift = 0;
		break;
	default:
		return -1;
		break;
	}

	if(level_left < 0) 
		level_left = 0;
	else if(level_left > max_level) 
		level_left = max_level;

	if(level_right < 0) 
		level_right = 0;
	else if(level_right > max_level) 
		level_right = max_level;

	zlox_sb16_mixer_set(reg_left, level_left << shift);
	if(reg_left != reg_right)
		zlox_sb16_mixer_set(reg_right, level_right << shift);
	return 0;
}

ZLOX_SINT32 zlox_sb16_mixer_set(ZLOX_SINT32 reg, ZLOX_SINT32 data)
{
	ZLOX_SINT32 i;

	zlox_outb(ZLOX_SB_MIXER_REG, reg);
	for(i = 0; i < 100; i++)
		;
	zlox_outb(ZLOX_SB_MIXER_DATA, data);

	return 0;
}

ZLOX_SINT32 zlox_sb16_mixer_get(ZLOX_SINT32 reg)
{
	ZLOX_SINT32 i;

	zlox_outb(ZLOX_SB_MIXER_REG, reg);
	for(i = 0; i < 100; i++)
		;
	return zlox_inb(ZLOX_SB_MIXER_DATA) & 0xff;
}

