#include "lodepng.h"
#include <stdlib.h>
#include "my_windows.h"

MY_WINDOW showpng = {0};
unsigned char * image;
size_t jump = 1;
unsigned img_w, img_h;

SINT32 showpng_win_callback (TASK_MSG * msg, MY_WINDOW * my_window)
{
	MY_RECT * tr;
	TASK * tmptask;
	switch(msg->type)
	{
	case MT_CREATE_MY_WINDOW:
		my_window->has_title = TRUE;
		my_window->title_rect.x = 0;
		my_window->title_rect.y = 0;
		my_window->title_rect.width = showpng.width - 20;
		my_window->title_rect.height = 20;
		tr = &my_window->title_rect;
		for (UINT16 y = tr->y; y < (tr->y + tr->height); y++) {
			for (UINT16 x = tr->x; x < (tr->x + tr->width); x++) {
				if(x == tr->x || x == (tr->x + tr->width - 1) || 
					y == tr->y || y == (tr->y + tr->height - 1))
					my_window->bitmap[x + y * showpng.width] = 0xFF000000 | (0xff * 0x10000) | (0xff * 0x100) | 0xff;
				else
					my_window->bitmap[x + y * showpng.width] = 0xFF000000 | (0x28 * 0x10000) | (0x31 * 0x100) | 0xE8;
			}
		}
		my_window->close_btn_rect.x = my_window->title_rect.width;
		my_window->close_btn_rect.y = 0;
		my_window->close_btn_rect.width = my_window->title_rect.height;
		my_window->close_btn_rect.height = my_window->title_rect.height;
		tr = &my_window->close_btn_rect;
		for (UINT16 y = tr->y; y < (tr->y + tr->height); y++) {
			for (UINT16 x = tr->x; x < (tr->x + tr->width); x++) {
				if(x == tr->x || x == (tr->x + tr->width - 1) || 
					y == tr->y || y == (tr->y + tr->height - 1))
					my_window->bitmap[x + y * showpng.width] = 0xFF000000 | (0xff * 0x10000) | (0xff * 0x100) | 0xff;
				else
					my_window->bitmap[x + y * showpng.width] = 0xFF000000 | (0xF7 * 0x10000) | (0x0B * 0x100) | 0x26;
			}
		}
		for (UINT16 y = my_window->title_rect.height, y2 = 0; y < showpng.height; y++, y2 += jump) 
		{
			for (UINT16 x = 0, x2 = 0; x < showpng.width; x++, x2 += jump) 
			{
				if(x == 1)
					x2 = 0;
				if(x == 0 || x == (showpng.width - 1) || y == 0 || y == (showpng.height - 1))
					my_window->bitmap[x + y * showpng.width] = 0xFF000000 | (0xff * 0x10000) | (0xff * 0x100) | 0xff;
				else
					my_window->bitmap[x + y * showpng.width] = 0xFF000000 | 
										(image[4 * y2 * img_w + 4 * x2 + 0] * 0x10000) | 
										(image[4 * y2 * img_w + 4 * x2 + 1] * 0x100) | 
										image[4 * y2 * img_w + 4 * x2 + 2];
			}
		}
		free(image);
		image = NULL;
		tmptask = (TASK *)my_window->task;
		tmptask = tmptask->parent;
		if(tmptask->status == TS_WAIT)
		{
			printf("now wake up the parent task!\n");
			tmptask->status = TS_RUNNING;
		}
		return 0;
		break;
	default:
		break;
	}
	return -1;
}

BOOL detect_filename(char * input, char ** tmp_name)
{
	SINT32 len =strlen(input);
	BOOL need_add = TRUE;
	for(SINT32 i=0;i<len;i++)
	{
		if(input[i]=='/')
			need_add = FALSE;
	}
	FS_NODE * fs_root = (FS_NODE *)syscall_get_fs_root();
	FS_NODE * fsnode = (FS_NODE *)malloc(sizeof(FS_NODE));
	SINT32 ret_int = (SINT32)syscall_finddir_fs_safe(fs_root, input, fsnode);
	(*tmp_name) = NULL;
	if(ret_int == -1)
	{
		if(need_add == FALSE)
		{
			free(fsnode);
			return FALSE;
		}
		char hd_img[10] = "hd/img/";
		SINT32 len_hd_img = strlen(hd_img);
		char * tmp = (char *)malloc((len_hd_img + len + 5));
		memcpy((UINT8 *)tmp, (UINT8 *)hd_img, len_hd_img);
		memcpy((UINT8 *)(tmp + len_hd_img), (UINT8 *)input, len);
		tmp[len_hd_img + len] = '\0';
		ret_int = (SINT32)syscall_finddir_fs_safe(fs_root, tmp, fsnode);
		if(ret_int == -1)
		{
			free(fsnode);
			free(tmp);
			return FALSE;
		}
		(*tmp_name) = tmp;
	}
	free(fsnode);
	return TRUE;
}

int main(VOID * task, int argc, char * argv[])
{
  	unsigned w, h, x, y;
	
	if(argc != 2)
	{
		printf("usage: showpng pngFileName");
		return -1;
	}
	char * tmp = NULL;
	if(detect_filename(argv[1], &tmp) == FALSE)
	{
		printf("the file \"%s\" is not exists\n", argv[1]);
		return -1;
	}
	if(tmp == NULL)
		tmp = argv[1];
	printf("now load and decode %s\n", tmp);
	unsigned error = lodepng_decode32_file(&image, &w, &h, tmp);
	/*stop if there is an error*/
	if(error)
	{
		printf("decoder error %u: %s", error, lodepng_error_text(error));
		if(tmp != argv[1])
			free(tmp);
		return 0;
	}
	if(tmp != argv[1])
		free(tmp);

	/*avoid too large window size by downscaling large image*/
	if(w / 1024 >= jump) jump = w / 1024 + 1;
	if(h / 1024 >= jump) jump = h / 1024 + 1;

	img_w = w;
	img_h = h;

	VOID * hshowpng = NULL;
	showpng.x = 310;
	showpng.y = 160;
	showpng.width = w / jump + 2;
	showpng.height = h / jump + 21;
	showpng.mywin_callback = (MY_WINDOW_CALLBACK)showpng_win_callback;
	showpng.task = task;
	hshowpng = (VOID *)syscall_create_my_window(&showpng);
	TASK_MSG msg;
	int ret;
	while(TRUE)
	{
		ret = syscall_get_tskmsg(task,&msg,TRUE);
		if(ret != 1)
		{
			syscall_idle_cpu();
			continue;
		}
		switch(msg.type)
		{
		case MT_CREATE_MY_WINDOW:
			syscall_dispatch_win_msg(&msg, hshowpng);
			break;
		case MT_CLOSE_MY_WINDOW:
			return 0;
			break;
		default:
			break;
		}
	}
	return 0;
}

