/*zlox_sb16.h the sb16 header*/

#ifndef _ZLOX_SB16_H_
#define _ZLOX_SB16_H_

#include "zlox_common.h"

#define ZLOX_SB_TIMEOUT 32000 /* timeout count */

/* IRQ, base address and DMA channels */
#define ZLOX_SB_IRQ 5
#define ZLOX_SB_BASE_ADDR 0x220

#define ZLOX_SB_DMA_8 1 /* 0, 1, 3 */
#define ZLOX_SB_DMA_16 5 /* 5, 6, 7 */

#define ZLOX_SB_DEFAULT_SPEED 22050 /* Sample rate */
#define ZLOX_SB_DEFAULT_BITS 8 /* Nr. of bits */
#define ZLOX_SB_DEFAULT_SIGN 0 /* 0 = unsigned, 1 = signed */
#define ZLOX_SB_DEFAULT_STEREO 0 /* 0 = mono, 1 = stereo */

/* DMA port addresses */
#define ZLOX_SB_DMA8_ADDR ((ZLOX_SB_DMA_8 & 3) << 1) + 0x00
#define ZLOX_SB_DMA8_COUNT ((ZLOX_SB_DMA_8 & 3) << 1) + 0x01
#define ZLOX_SB_DMA8_MASK 0x0A
#define ZLOX_SB_DMA8_MODE 0x0B
#define ZLOX_SB_DMA8_CLEAR 0x0C

/* If after this preprocessing stuff ZLOX_SB_DMA8_PAGE is not defined
 * the 8-bit DMA channel specified is not valid
 */
#if ZLOX_SB_DMA_8 == 0
#  define ZLOX_SB_DMA8_PAGE	0x87
#else 
#  if ZLOX_SB_DMA_8 == 1
#    define ZLOX_SB_DMA8_PAGE 0x83
#  else 
#    if ZLOX_SB_DMA_8 == 3
#      define ZLOX_SB_DMA8_PAGE 0x82
#    endif
#  endif
#endif

#define ZLOX_SB_DMA16_ADDR ((ZLOX_SB_DMA_16 & 3) << 2) + 0xC0
#define ZLOX_SB_DMA16_COUNT ((ZLOX_SB_DMA_16 & 3) << 2) + 0xC2 
#define ZLOX_SB_DMA16_MASK 0xD4
#define ZLOX_SB_DMA16_MODE 0xD6
#define ZLOX_SB_DMA16_CLEAR 0xD8

/* If after this preprocessing stuff ZLOX_SB_DMA16_PAGE is not defined
 * the 16-bit DMA channel specified is not valid
 */
#if ZLOX_SB_DMA_16 == 5
#  define ZLOX_SB_DMA16_PAGE 0x8B
#else 
#  if ZLOX_SB_DMA_16 == 6
#    define ZLOX_SB_DMA16_PAGE 0x89
#  else 
#    if ZLOX_SB_DMA_16 == 7
#      define ZLOX_SB_DMA16_PAGE 0x8A
#    endif
#  endif
#endif

/* DMA modes */
#define ZLOX_SB_DMA16_AUTO_PLAY 0x58 + (ZLOX_SB_DMA_16 & 3)
#define ZLOX_SB_DMA16_AUTO_REC 0x54 + (ZLOX_SB_DMA_16 & 3)
#define ZLOX_SB_DMA8_AUTO_PLAY 0x58 + ZLOX_SB_DMA_8
#define ZLOX_SB_DMA8_AUTO_REC 0x54 + ZLOX_SB_DMA_8

/* IO ports for sound blaster */
#define ZLOX_SB_DSP_RESET 0x6 + ZLOX_SB_BASE_ADDR
#define ZLOX_SB_DSP_READ 0xA + ZLOX_SB_BASE_ADDR
#define ZLOX_SB_DSP_COMMAND	0xC + ZLOX_SB_BASE_ADDR
#define ZLOX_SB_DSP_STATUS 0xC + ZLOX_SB_BASE_ADDR
#define ZLOX_SB_DSP_DATA_AVL 0xE + ZLOX_SB_BASE_ADDR
#define ZLOX_SB_DSP_DATA16_AVL 0xF + ZLOX_SB_BASE_ADDR
#define ZLOX_SB_MIXER_REG 0x4 + ZLOX_SB_BASE_ADDR
#define ZLOX_SB_MIXER_DATA 0x5 + ZLOX_SB_BASE_ADDR

/* DSP Commands */
#define ZLOX_SB_DSP_INPUT_RATE 0x42  /* set input sample rate */
#define ZLOX_SB_DSP_OUTPUT_RATE 0x41  /* set output sample rate */
#define ZLOX_SB_DSP_CMD_SPKON 0xD1  /* set speaker on */
#define ZLOX_SB_DSP_CMD_SPKOFF 0xD3  /* set speaker off */
#define ZLOX_SB_DSP_CMD_DMA8PAUSE 0xD0 /* pause DMA 8-bit operation */  
#define ZLOX_SB_DSP_CMD_DMA8CONT 0xD4 /* continue DMA 8-bit operation */
#define ZLOX_SB_DSP_CMD_DMA8EXIT 0xDA /* exit DMA 8-bit operation */ 
#define ZLOX_SB_DSP_CMD_DMA16PAUSE 0xD5  /* pause DMA 16-bit operation */
#define ZLOX_SB_DSP_CMD_DMA16CONT 0xD6  /* continue DMA 16-bit operation */
#define ZLOX_SB_DSP_CMD_DMA16EXIT 0xD9 /* exit DMA 16-bit operation */ 
#define ZLOX_SB_DSP_GET_VERSION 0xE1 /* get version number of DSP */
#define ZLOX_SB_DSP_CMD_8BITAUTO_IN 0xCE  /* 8 bit auto-initialized input */
#define ZLOX_SB_DSP_CMD_8BITAUTO_OUT 0xC6  /* 8 bit auto-initialized output */
#define ZLOX_SB_DSP_CMD_16BITAUTO_IN 0xBE  /* 16 bit auto-initialized input */
#define ZLOX_SB_DSP_CMD_16BITAUTO_OUT 0xB6  /* 16 bit auto-initialized output */

/* DSP Modes */
#define ZLOX_SB_DSP_MODE_MONO_US 0x00  /* Mono unsigned */
#define ZLOX_SB_DSP_MODE_MONO_S 0x10  /* Mono signed */
#define ZLOX_SB_DSP_MODE_STEREO_US 0x20  /* Stereo unsigned */
#define ZLOX_SB_DSP_MODE_STEREO_S 0x30  /* Stereo signed */

/* MIXER reg index */
#define ZLOX_SB_MIXER_DAC_LEVEL 0x04  /* Used for detection only */
#define ZLOX_SB_MIXER_MASTER_LEFT 0x30  /* Master volume left */
#define ZLOX_SB_MIXER_MASTER_RIGHT 0x31  /* Master volume right */
#define ZLOX_SB_MIXER_DAC_LEFT 0x32  /* Dac level left */
#define ZLOX_SB_MIXER_DAC_RIGHT 0x33  /* Dac level right */
#define ZLOX_SB_MIXER_FM_LEFT 0x34  /* Fm level left */
#define ZLOX_SB_MIXER_FM_RIGHT 0x35  /* Fm level right */
#define ZLOX_SB_MIXER_CD_LEFT 0x36  /* Cd audio level left */
#define ZLOX_SB_MIXER_CD_RIGHT 0x37  /* Cd audio level right */
#define ZLOX_SB_MIXER_LINE_LEFT 0x38  /* Line in level left */
#define ZLOX_SB_MIXER_LINE_RIGHT 0x39  /* Line in level right */
#define ZLOX_SB_MIXER_MIC_LEVEL 0x3A  /* Microphone level */
#define ZLOX_SB_MIXER_PC_LEVEL 0x3B  /* Pc speaker level */
#define ZLOX_SB_MIXER_OUTPUT_CTRL 0x3C  /* Output control */
#define ZLOX_SB_MIXER_GAIN_OUT_LEFT 0x41  /* Output gain control left */
#define ZLOX_SB_MIXER_GAIN_OUT_RIGHT 0x42  /* Output gain control rigth */
#define ZLOX_SB_MIXER_AGC 0x43  /* Automatic gain control */
#define ZLOX_SB_MIXER_TREBLE_LEFT 0x44  /* Treble left */
#define ZLOX_SB_MIXER_TREBLE_RIGHT 0x45  /* Treble right */
#define ZLOX_SB_MIXER_BASS_LEFT 0x46  /* Bass left */
#define ZLOX_SB_MIXER_BASS_RIGHT 0x47  /* Bass right */
#define ZLOX_SB_MIXER_SET_IRQ 0x80  /* Set irq number */
#define ZLOX_SB_MIXER_SET_DMA 0x81  /* Set DMA channels */
#define ZLOX_SB_MIXER_IRQ_STATUS 0x82  /* Irq status */

/* DSP constants */
#define ZLOX_SB_DSP_MAX_SPEED 44100 /* Max sample speed in Hz */
#define ZLOX_SB_DSP_MIN_SPEED 5000 /* Min sample speed in Hz */

#define ZLOX_SB_DSP_FRAG_SIZE 32 * 1024
#define ZLOX_SB_DMA_SIZE 64 * 1024

#define ZLOX_SB_WRITE_DMA 1

#define ZLOX_SB_DEFAULT_FILL_DATA 0x00

ZLOX_SINT32 zlox_sb16_init();
ZLOX_SINT32 zlox_sb16_dsp_reset();
ZLOX_SINT32 zlox_sb16_dsp_pause();
ZLOX_SINT32 zlox_sb16_dsp_continue();
ZLOX_SINT32 zlox_sb16_dsp_exit();
ZLOX_SINT32 zlox_sb16_dsp_start(ZLOX_UINT32 DmaPhys, ZLOX_SINT32 DmaMode);
ZLOX_SINT32 zlox_sb16_dsp_set_bits(ZLOX_UINT32 bits);
ZLOX_SINT32 zlox_sb16_dsp_set_sign(ZLOX_UINT32 sign);
ZLOX_SINT32 zlox_sb16_dsp_set_stereo(ZLOX_UINT32 stereo);
ZLOX_SINT32 zlox_sb16_dsp_set_speed(ZLOX_UINT32 speed, ZLOX_BOOL need_cmd);
ZLOX_SINT32 zlox_sb16_set_dma_phys(ZLOX_UINT32 DmaPhys, ZLOX_BOOL need_clear);
ZLOX_UINT32 zlox_sb16_get_dma_phys();
ZLOX_SINT32 zlox_sb16_set_task(ZLOX_TASK * task);
ZLOX_VOID * zlox_sb16_get_task();
/*=========================================================================*
 *				mixer				   
 *=========================================================================*/
ZLOX_SINT32 zlox_sb16_mixer_init();

#endif // _ZLOX_SB16_H_

