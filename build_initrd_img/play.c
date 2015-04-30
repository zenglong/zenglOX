// play.c -- 播放指定的wav音频文件

#include "common.h"
#include "syscall.h"
#include "task.h"
#include "fs.h"
#include "audio.h"

struct _WAV_FILE_HEADER
{
	char ChunkID[4];
	UINT32 ChunkSize;
	char Format[4];
	char Subchunk1ID[4];
	UINT32 Subchunk1Size;
	UINT16 AudioFormat;
	UINT16 NumChannels;
	UINT32 SampleRate;
	UINT32 ByteRate;
	UINT16 BlockAlign;
	UINT16 BitsPerSample;
	char Subchunk2ID[4];
	UINT32 Subchunk2Size;
}__attribute__((packed));

typedef struct _WAV_FILE_HEADER WAV_FILE_HEADER;

int wav_file_detect(UINT8 * buf, UINT32 * bits, UINT32 * sign, 
				UINT32 * stereo, UINT32 * speed, UINT32 * datasize, UINT8 ** data)
{
	WAV_FILE_HEADER * wav = (WAV_FILE_HEADER *)buf;
	if(strcmpn(wav->ChunkID, "RIFF", 4) != 0)
	{
		syscall_cmd_window_write("invalid wav file: ChunkID is not \"RIFF\"");
		return -1;
	}
	if(strcmpn(wav->Format, "WAVE", 4) != 0)
	{
		syscall_cmd_window_write("invalid wav file: Format is not \"WAVE\"");
		return -1;
	}
	if(strcmpn(wav->Subchunk1ID, "fmt ", 4) != 0)
	{
		syscall_cmd_window_write("invalid wav file: Subchunk1ID is not \"fmt \"");
		return -1;
	}
	if(wav->Subchunk1Size != 16)
	{
		syscall_cmd_window_write("Unsupported wav file: Subchunk1Size is not 16");
		return -1;
	}
	if(wav->AudioFormat != 1)
	{
		syscall_cmd_window_write("Unsupported wav file: AudioFormat is not 1, I only support PCM"
					", I don't support compression");
		return -1;
	}
	if((wav->NumChannels != 1) && (wav->NumChannels != 2))
	{
		syscall_cmd_window_write("Unsupported wav file: NumChannels is not 1 or 2, I only support Mono "
					"or Stereo wav file");
		return -1;
	}
	else
	{
		(*stereo) = (wav->NumChannels == 1 ? 0 : 1); 
	}
	if((wav->SampleRate < 5000) || (wav->SampleRate > 44100))
	{
		syscall_cmd_window_write("Unsupported wav file: SampleRate must be in 5000 to 44100");
		return -1;
	}
	(*speed) = wav->SampleRate;
	if((wav->BitsPerSample != 8) && (wav->BitsPerSample != 16))
	{
		syscall_cmd_window_write("Unsupported wav file: BitsPerSample must be 8 bit or 16 bit");
		return -1;
	}
	(*bits) = (wav->BitsPerSample == 8 ? 8 : 16);
	(*sign) = (wav->BitsPerSample == 8 ? 0 : 1);
	if(strcmpn(wav->Subchunk2ID, "data", 4) != 0)
	{
		syscall_cmd_window_write("invalid wav file: Subchunk2ID is not \"data\"");
		return -1;
	}
	(*datasize) = wav->Subchunk2Size;
	(*data) = buf + sizeof(WAV_FILE_HEADER);
	return 0;
}

int main(VOID * task, int argc, char * argv[])
{
	TASK_MSG msg;
	BOOL exit = FALSE;
	FS_NODE * fsnode = NULL;
	FS_NODE * fs_root = NULL;
	int ret = 0;
	AUD_EXTRA_DATA extra_data;
	memset((UINT8 *)&extra_data, 0, sizeof(AUD_EXTRA_DATA));
	syscall_audio_ctrl(ACT_DETECT_AUD_EXIST, &extra_data);
	if(!extra_data.audio.exist)
	{
		syscall_cmd_window_write("sound card not detected!");
		goto end;
	}
	if(argc != 2)
	{
		syscall_cmd_window_write("usage: play wavfile");
		goto end;
	}
	fsnode = (FS_NODE *)syscall_umalloc(sizeof(FS_NODE));
	fs_root = (FS_NODE *)syscall_get_fs_root();
	ret = syscall_finddir_fs_safe(fs_root, argv[1], fsnode);
	if (ret == -1)
	{
		syscall_cmd_window_write("\"");
		syscall_cmd_window_write(argv[1]);
		syscall_cmd_window_write("\" is not exist in your file system ");
		goto end;
	}
	else if ((fsnode->flags & 0x7) == FS_FILE)
	{
		CHAR * buf = (CHAR *)syscall_umalloc(fsnode->length + 1);
		UINT32 bits, sign, stereo, speed, datasize;
		UINT8 * data;
		syscall_read_fs(fsnode,0,fsnode->length,buf);
		buf[fsnode->length] = '\0';
		if(wav_file_detect((UINT8 *)buf, &bits, &sign, &stereo, 
					&speed, &datasize, &data) == -1)
		{
			syscall_ufree(buf);
			goto end;
		}
		if(syscall_audio_set_databuf(data, (int)datasize) == -2)
		{
			syscall_cmd_window_write("At the same time, there is another audio player is working, "
						"you can't play now!");
			syscall_ufree(buf);
			goto end;
		}
		syscall_ufree(buf);
		syscall_audio_set_args(bits, sign, stereo, speed, task);
		if(bits == 8)
		{
			extra_data.audio.FillData = 0x80;
		}
		else
			extra_data.audio.FillData = 0x0;
		syscall_audio_ctrl(ACT_SET_FILL_DATA, &extra_data);
		syscall_audio_play();
		syscall_cmd_window_write("play ");
		syscall_cmd_window_write(argv[1]);
		syscall_cmd_window_write("...");
	}
	else if ((fsnode->flags & 0x7) == FS_DIRECTORY)
	{
		syscall_cmd_window_write("\"");
		syscall_cmd_window_write("test.wav");
		syscall_cmd_window_write("\" is a directory ");
		goto end;
	}

	syscall_set_input_focus(task);
	while(TRUE)
	{
		ret = syscall_get_tskmsg(task,&msg,TRUE);
		if(ret != 1)
		{
			syscall_idle_cpu();
			continue;
		}

		if(msg.type == MT_AUDIO_INT)
		{
			//syscall_cmd_window_write("audio int...");
			int play_size = syscall_audio_ctrl(ACT_GET_PLAY_SIZE, NULL);
			syscall_cmd_window_write("[");
			syscall_cmd_window_write_dec((int)(((double)play_size / 
							(double)fsnode->length) * 100));
			syscall_cmd_window_write("%]...");
			continue;
		}
		else if(msg.type == MT_AUDIO_END)
		{
			syscall_cmd_window_write("audio end!");
			break;
		}
		else if(msg.type == MT_KEYBOARD && msg.keyboard.type == MKT_ASCII)
		{
			switch(msg.keyboard.ascii)
			{
			case 'p':
			case 'P':
				syscall_cmd_window_write("pause...");
				syscall_audio_ctrl(ACT_PAUSE, NULL);
				break;
			case 'c':
			case 'C':
				syscall_cmd_window_write("continue...");
				syscall_audio_ctrl(ACT_CONTINUE, NULL);
				break;
			case 0x1B: //ESC
				syscall_cmd_window_write("exit");
				syscall_audio_ctrl(ACT_EXIT, NULL);
				exit = TRUE;
				break;
			}
			if(exit)
				break;
		}
	}

end:
	if(fsnode != NULL)
		syscall_ufree(fsnode);
	return 0;
}

