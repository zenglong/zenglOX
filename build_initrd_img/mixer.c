// mixer.c -- 对音量进行调节的程式

#include "common.h"
#include "syscall.h"
#include "audio.h"

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(argc);
	UNUSED(argv);
	UNUSED(task);
	int init_ret = syscall_audio_ctrl(ACT_MIXER_INIT, NULL);
	AUD_EXTRA_DATA extra_data;
	memset((UINT8 *)&extra_data, 0, sizeof(AUD_EXTRA_DATA));
	if(init_ret == -1)
	{
		syscall_cmd_window_write("Mixer not exist!");
		return -1;
	}
	else
		syscall_cmd_window_write("Mixer detected!\n");
	if(argc == 2)
	{
		if(strcmp(argv[1], "-l") == 0)
		{
			syscall_audio_ctrl(ACT_MIXER_GET_VOLUME, &extra_data);
			syscall_cmd_window_write("Mixer Master left:");
			syscall_cmd_window_write_dec(extra_data.mixer.master_left);
			syscall_cmd_window_write("  Mixer Master right:");
			syscall_cmd_window_write_dec(extra_data.mixer.master_right);
			syscall_cmd_window_write("\nMixer Voice left:");
			syscall_cmd_window_write_dec(extra_data.mixer.voice_left);
			syscall_cmd_window_write("  Mixer Voice right:");
			syscall_cmd_window_write_dec(extra_data.mixer.voice_right);
			syscall_cmd_window_write("\nMixer Midi left:");
			syscall_cmd_window_write_dec(extra_data.mixer.midi_left);
			syscall_cmd_window_write("  Mixer Midi right:");
			syscall_cmd_window_write_dec(extra_data.mixer.midi_right);
			syscall_cmd_window_write("\nMixer CD left:");
			syscall_cmd_window_write_dec(extra_data.mixer.cd_left);
			syscall_cmd_window_write("  Mixer CD right:");
			syscall_cmd_window_write_dec(extra_data.mixer.cd_right);
			syscall_cmd_window_write("\nMixer Line left:");
			syscall_cmd_window_write_dec(extra_data.mixer.line_left);
			syscall_cmd_window_write("  Mixer Line right:");
			syscall_cmd_window_write_dec(extra_data.mixer.line_right);
			syscall_cmd_window_write("\nMixer Mic Volume:");
			syscall_cmd_window_write_dec(extra_data.mixer.mic_volume);
			syscall_cmd_window_write("  Mixer PC Volume:");
			syscall_cmd_window_write_dec(extra_data.mixer.pc_volume);
			syscall_cmd_window_write("\nMixer Treble left:");
			syscall_cmd_window_write_dec(extra_data.mixer.treble_left);
			syscall_cmd_window_write("  Mixer Treble right:");
			syscall_cmd_window_write_dec(extra_data.mixer.treble_right);
			syscall_cmd_window_write("\nMixer Bass left:");
			syscall_cmd_window_write_dec(extra_data.mixer.bass_left);
			syscall_cmd_window_write("  Mixer Bass right:");
			syscall_cmd_window_write_dec(extra_data.mixer.bass_right);
			syscall_cmd_window_write("\nMixer output Gain left:");
			syscall_cmd_window_write_dec(extra_data.mixer.output_gain_left);
			syscall_cmd_window_write("  Mixer output Gain right:");
			syscall_cmd_window_write_dec(extra_data.mixer.output_gain_right);
			syscall_cmd_window_write("\nMixer output Ctrl:");
			syscall_cmd_window_write_dec(extra_data.mixer.output_ctrl);
		}
		else if(strcmp(argv[1], "-h") == 0)
			goto help;
		else
			goto fail;
	}
	else if(argc == 3)
	{
		if(strcmp(argv[1], "-mic") == 0)
		{
			extra_data.mixer.mic_volume =  (int)strToUInt(argv[2]);
			if(syscall_audio_ctrl(ACT_MIXER_SET_MIC, &extra_data) == 0)
				syscall_cmd_window_write("set Mixer Mic Volume successed!");
			else
				syscall_cmd_window_write("set Mixer Mic Volume failed!");
		}
		else if(strcmp(argv[1], "-pc") == 0)
		{
			extra_data.mixer.pc_volume =  (int)strToUInt(argv[2]);
			if(syscall_audio_ctrl(ACT_MIXER_SET_PC, &extra_data) == 0)
				syscall_cmd_window_write("set Mixer PC Volume successed!");
			else
				syscall_cmd_window_write("set Mixer PC Volume failed!");
		}
		else if(strcmp(argv[1], "-ctrl") == 0)
		{
			extra_data.mixer.output_ctrl =  (int)strToUInt(argv[2]);
			if(syscall_audio_ctrl(ACT_MIXER_SET_OUT_CTRL, &extra_data) == 0)
				syscall_cmd_window_write("set Mixer output Ctrl successed!");
			else
				syscall_cmd_window_write("set Mixer output Ctrl failed!");
		}
		else 
			goto fail;
	}
	else if(argc == 4)
	{
		int level_left = (int)strToUInt(argv[2]);
		int level_right = (int)strToUInt(argv[3]);
		if(strcmp(argv[1], "-mv") == 0)
		{
			extra_data.mixer.master_left = level_left;
			extra_data.mixer.master_right = level_right;
			if(syscall_audio_ctrl(ACT_MIXER_SET_MASTER, &extra_data) == 0)
				syscall_cmd_window_write("set Mixer Master successed!");
			else
				syscall_cmd_window_write("set Mixer Master failed!");
			extra_data.mixer.voice_left = level_left;
			extra_data.mixer.voice_right = level_right;
			if(syscall_audio_ctrl(ACT_MIXER_SET_VOICE, &extra_data) == 0)
				syscall_cmd_window_write("\nset Mixer Voice successed!");
			else
				syscall_cmd_window_write("\nset Mixer Voice failed!");
		}
		else if(strcmp(argv[1], "-m") == 0)
		{
			extra_data.mixer.master_left = level_left;
			extra_data.mixer.master_right = level_right;
			if(syscall_audio_ctrl(ACT_MIXER_SET_MASTER, &extra_data) == 0)
				syscall_cmd_window_write("set Mixer Master successed!");
			else
				syscall_cmd_window_write("set Mixer Master failed!");
		}
		else if(strcmp(argv[1], "-v") == 0)
		{
			extra_data.mixer.voice_left = level_left;
			extra_data.mixer.voice_right = level_right;
			if(syscall_audio_ctrl(ACT_MIXER_SET_VOICE, &extra_data) == 0)
				syscall_cmd_window_write("set Mixer Voice successed!");
			else
				syscall_cmd_window_write("set Mixer Voice failed!");
		}
		else if(strcmp(argv[1], "-midi") == 0)
		{
			extra_data.mixer.midi_left = level_left;
			extra_data.mixer.midi_right = level_right;
			if(syscall_audio_ctrl(ACT_MIXER_SET_MIDI, &extra_data) == 0)
				syscall_cmd_window_write("set Mixer Midi successed!");
			else
				syscall_cmd_window_write("set Mixer Midi failed!");
		}
		else if(strcmp(argv[1], "-cd") == 0)
		{
			extra_data.mixer.cd_left = level_left;
			extra_data.mixer.cd_right = level_right;
			if(syscall_audio_ctrl(ACT_MIXER_SET_CD, &extra_data) == 0)
				syscall_cmd_window_write("set Mixer CD successed!");
			else
				syscall_cmd_window_write("set Mixer CD failed!");
		}
		else if(strcmp(argv[1], "-line") == 0)
		{
			extra_data.mixer.line_left = level_left;
			extra_data.mixer.line_right = level_right;
			if(syscall_audio_ctrl(ACT_MIXER_SET_LINE, &extra_data) == 0)
				syscall_cmd_window_write("set Mixer Line successed!");
			else
				syscall_cmd_window_write("set Mixer Line failed!");
		}
		else if(strcmp(argv[1], "-treble") == 0)
		{
			extra_data.mixer.treble_left = level_left;
			extra_data.mixer.treble_right = level_right;
			if(syscall_audio_ctrl(ACT_MIXER_SET_TREBLE, &extra_data) == 0)
				syscall_cmd_window_write("set Mixer Treble successed!");
			else
				syscall_cmd_window_write("set Mixer Treble failed!");
		}
		else if(strcmp(argv[1], "-bass") == 0)
		{
			extra_data.mixer.bass_left = level_left;
			extra_data.mixer.bass_right = level_right;
			if(syscall_audio_ctrl(ACT_MIXER_SET_BASS, &extra_data) == 0)
				syscall_cmd_window_write("set Mixer Bass successed!");
			else
				syscall_cmd_window_write("set Mixer Bass failed!");
		}
		else if(strcmp(argv[1], "-gain") == 0)
		{
			extra_data.mixer.output_gain_left = level_left;
			extra_data.mixer.output_gain_right = level_right;
			if(syscall_audio_ctrl(ACT_MIXER_SET_OUT_GAIN, &extra_data) == 0)
				syscall_cmd_window_write("set Mixer output Gain successed!");
			else
				syscall_cmd_window_write("set Mixer output Gain failed!");
		}
		else 
			goto fail;
	}
	else
		goto fail;
	return 0;
fail:
	syscall_cmd_window_write("usage: mixer [-h][-l][-mv left right][-m left right][-v left right][-midi left right]"
			"[-cd left right][-line left right][-treble left right][-bass left right][-gain left right]"
			"[-mic volume][-pc volume][-ctrl output-ctrl]\n");
	return -1;
help:
	syscall_cmd_window_write("usage: mixer [-h][-l][-mv left right][-m left right][-v left right][-midi left right]"
			"[-cd left right][-line left right][-treble left right][-bass left right][-gain left right]"
			"[-mic volume][-pc volume][-ctrl output-ctrl]\n\n"
			"-h: show help; -l: list all volumes\n"
			"-mv: set master and voice volume; -m: set master volume\n"
			"-v: set voice volume; -midi: set midi volume\n"
			"-cd: set CD volume; -line: set line volume\n"
			"-treble: set Treble; -bass: set Bass\n"
			"-gain: set output Gain; -mic: set mic volume\n"
			"-pc: set PC volume; -ctrl: set output mixer switches\n");
	return 0;
}

