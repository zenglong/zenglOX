#include <stdlib.h>

#define FILE_EXT_BUF_SIZE 1024

BOOL single_line_out = FALSE;
BOOL input_focus_flag = FALSE;
VOID * cur_task = NULL;
char file_tmp_name[128];
unsigned int libc_random_seed = 0;
char tmpnam_bbuf[20] = {0};
char tmpnam_random_char[18] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '_'};
FILE * libc_tmpfile = NULL;

SINT32 strnlen(const char *s, SINT32 count)
{
	const char *sc;

	for (sc = s; count-- && *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}

int isspace(int x)
{
	if(x==' '|| x=='\t'||x=='\n'|| x=='\f'|| x=='\b'|| x=='\r')
		return 1;
	else 
		return 0;
}

int isdigit(int x)
{
	if(x <= '9' && x >= '0')        
		return 1;
	else
		return 0;
}

int atoi(const char *nptr)
{
	int c; /* current char */
	int total; /* current total */
	int sign; /* if '-', then negative, otherwise positive */

	/* skip whitespace */
	while ( isspace((int)(unsigned char)*nptr) )
		++nptr;

	c = (int)(unsigned char)*nptr++;
	sign = c; /* save sign indication */
	if (c == '-' || c == '+')
		c = (int)(unsigned char)*nptr++; /* skip sign */

	total = 0;

	while (isdigit(c)) {
		total = 10 * total + (c - '0'); /* accumulate digit */
		c = (int)(unsigned char)*nptr++; /* get next char */
        }

	if (sign == '-')
		return -total;
	else
		return total; /* return result, negated if necessary */
}

void initscr()
{
	syscall_monitor_clear();
}

void clrscr()
{
	syscall_monitor_clear();
}

void endwin()
{
	if(single_line_out)
	{
		single_line_out = FALSE;
		syscall_monitor_set_single(single_line_out);
	}
	move(24, 0);
}

void move(int y, int x)
{
	syscall_monitor_set_cursor((UINT8)x, (UINT8)y);
}

void cputs_line(const char * str)
{
	if(!single_line_out)
	{
		single_line_out = TRUE;
		syscall_monitor_set_single(single_line_out);
	}
	syscall_monitor_write(str);
}

void cputs(const char * str)
{
	if(single_line_out)
	{
		single_line_out = FALSE;
		syscall_monitor_set_single(single_line_out);
	}
	syscall_monitor_write(str);
}

void putch(char c)
{
	if(single_line_out)
	{
		single_line_out = FALSE;
		syscall_monitor_set_single(single_line_out);
	}
	syscall_monitor_put(c);
}

int getch(void)
{
	if(!input_focus_flag)
	{
		cur_task = (VOID *)syscall_get_currentTask();
		syscall_set_input_focus(cur_task);
		input_focus_flag = TRUE;
	}

	TASK_MSG msg;
	int ret;
	while(TRUE)
	{
		ret = syscall_get_tskmsg(cur_task, &msg,TRUE);
		if(ret != 1)
		{
			syscall_idle_cpu();
			continue;
		}
		if(msg.type == MT_KEYBOARD)
		{
			if(msg.keyboard.type == MKT_ASCII)
				return (int)msg.keyboard.ascii;
			else if(msg.keyboard.type == MKT_KEY)
				return (int)msg.keyboard.key;
		}
	}
	return 0;
}

SINT32 libc_create_empty_file(FS_NODE * fs_root, char * name, FS_NODE * output)
{
	SINT32 name_len = strlen(name),j;
	BOOL flag = FALSE;
	if(name_len <= 0)
		return -1;
	if(output == NULL)
		return -1;

	strcpy(file_tmp_name,name);
	for(j=0;j < name_len;j++)
	{
		if(file_tmp_name[j] == '/')
		{
			file_tmp_name[j] = '\0';
			if(!flag)
				flag = TRUE;
		}
	}
	if(!flag)
	{
		SINT32 ret = (SINT32)syscall_writedir_fs_safe(fs_root, name, FS_FILE, output);
		if(ret == -1)
		{
			return -1;
		}
		return ret;
	}

	SINT32 tmp_name_offset = 0, tmp_name_len;
	CHAR * tmp_name = file_tmp_name + tmp_name_offset;
	SINT32 ret_int = 0;
	FS_NODE * tmp_node = fs_root;
	BOOL isLast = FALSE;
	while(TRUE)
	{
		ret_int = (SINT32)syscall_finddir_fs_safe(tmp_node, tmp_name, output);
		tmp_name_len = strlen(tmp_name);
		if((tmp_name_offset + tmp_name_len + 1) >= name_len + 1)
			isLast = TRUE;
		if(ret_int == -1)
		{
			if(isLast)
			{
				ret_int = (SINT32)syscall_writedir_fs_safe(tmp_node, tmp_name, FS_FILE, output);
				if(ret_int == -1)
				{
					return -1;
				}
				return ret_int;
			}
			else
			{
				ret_int = (SINT32)syscall_writedir_fs_safe(tmp_node, tmp_name, FS_DIRECTORY, output);
				if(ret_int == -1)
				{
					return -1;
				}
			}
		}
		else
		{
			if(isLast)
			{
				return -1;
			}
			else if((output->flags & 0x7) == FS_FILE)
			{
				return -1;
			}
		}
		tmp_node = output;
		tmp_name_offset += tmp_name_len + 1;
		if(tmp_name_offset < name_len + 1)
		{
			tmp_name = file_tmp_name + tmp_name_offset;
		}
		else
			break;
	}
	return ret_int;
}

FILE * fopen(char * filename, char * op_type_str)
{
	if(filename == NULL)
		return NULL;
	if(op_type_str == NULL)
		return NULL;

	FILE_OP_TYPE op_type;
	FS_NODE * fsnode = NULL;
	FILE * ret_file = NULL;

	if(strcmp(op_type_str, "r") == 0)
		op_type = FILE_OP_TP_RD;
	else if(strcmp(op_type_str, "w") == 0)
		op_type = FILE_OP_TP_WR;
	else if(strcmp(op_type_str, "rw") == 0)
		op_type = FILE_OP_TP_RW;
	else
		op_type = FILE_OP_TP_RD;

	FS_NODE * fs_root = (FS_NODE *)syscall_get_fs_root();
	fsnode = (FS_NODE *)syscall_umalloc(sizeof(FS_NODE));
	SINT32 ret_int = (SINT32)syscall_finddir_fs_safe(fs_root, filename, fsnode);
	if(ret_int == -1 && 
	   (op_type == FILE_OP_TP_WR || op_type == FILE_OP_TP_RW))
	{
		ret_int = libc_create_empty_file(fs_root, filename, fsnode);
		if(ret_int == -1)
			goto null_exit;
	}
	else if(ret_int == -1)
		goto null_exit;

	else if ((fsnode->flags & 0x7) != FS_FILE)
		goto null_exit;

	ret_file = (FILE *)syscall_umalloc(sizeof(FILE));
	memset((UINT8 *)ret_file, 0, sizeof(FILE));
	ret_file->fsnode = fsnode;
	ret_file->buf = (CHAR *)syscall_umalloc(fsnode->length + 1);
	ret_file->buf_size = fsnode->length + 1;
	syscall_read_fs(fsnode,0,fsnode->length,ret_file->buf);
	ret_file->buf[fsnode->length] = '\0';
	ret_file->op_type = op_type;
	ret_file->cur = 0;
	ret_file->sign = FILE_SIGN;
	ret_file->isdirty = FALSE;
	return ret_file;

null_exit:
	if(fsnode != NULL)
		syscall_ufree(fsnode);
	if(ret_file != NULL && ret_file->buf != NULL)
		syscall_ufree(ret_file->buf);
	if(ret_file != NULL)
		syscall_ufree(ret_file);
	return NULL;
}

int fclose(FILE * file)
{
	if(file == NULL)
		return -1;
	if(file->sign != FILE_SIGN)
		return -1;

	if(file->isdirty == TRUE)
	{
		syscall_write_fs(file->fsnode,0,file->fsnode->length,file->buf);
	}

	if(file->fsnode != NULL)
		syscall_ufree(file->fsnode);
	if(file->buf != NULL)
		syscall_ufree(file->buf);
	file->sign = 0;
	file->cur = 0;
	file->isdirty = FALSE;
	file->op_type = 0;
	file->buf_size = 0;
	syscall_ufree(file);
	return 0;
}

int fgetc(FILE * file)
{
	if(file == NULL)
		return -2;
	if(file->sign != FILE_SIGN)
		return -2;
	if(file->fsnode == NULL)
		return -2;
	if(file->buf == NULL)
		return -2;

	if(file->cur >= file->fsnode->length)
		return EOF;

	int ret = (int)file->buf[file->cur];
	file->cur++;
	return ret;
}

static CHAR * file_extend_buf(FILE * file, UINT32 ext_size)
{
	UINT32 orig_buf_size = file->buf_size;
	file->buf_size += ext_size;
	CHAR * tmp = (CHAR *)syscall_umalloc(file->buf_size);
	if(tmp == NULL)
		return NULL;
	memcpy((UINT8 *)tmp, (UINT8 *)file->buf, orig_buf_size);
	memset((UINT8 *)(tmp + orig_buf_size), 0, ext_size);
	syscall_ufree(file->buf);
	file->buf = tmp;
	return file->buf;
}

int fputc(int c, FILE * file)
{
	if(file == NULL)
		return -1;
	if(file->sign != FILE_SIGN)
		return -1;
	if(file->fsnode == NULL)
		return -1;
	if(file->buf == NULL)
		return -1;

	if(file->cur >= file->buf_size)
	{
		if(file_extend_buf(file, FILE_EXT_BUF_SIZE) == NULL)
			return -1;
	}
	file->buf[file->cur] = (CHAR)(c & 0xff);
	file->cur++;
	file->fsnode->length = file->cur;
	file->isdirty = TRUE;
	return (c & 0xff);
}

int fputs(const char * s, FILE * file)
{
	if(file == NULL)
		return -1;
	if(file->sign != FILE_SIGN)
		return -1;
	if(file->fsnode == NULL)
		return -1;
	if(file->buf == NULL)
		return -1;

	UINT32 len = (UINT32)strlen((char *)s);

	if(len == 0)
		return -1;

	// 加个10，可以有效的防止用户堆发生溢出!
	if(file->cur + len + 10 > file->buf_size)
	{
		UINT32 ext_size = ((file->cur + len + 10 - file->buf_size) / FILE_EXT_BUF_SIZE) + 1;
		ext_size *= FILE_EXT_BUF_SIZE;
		if(file_extend_buf(file, ext_size) == NULL)
			return -1;
	}
	strcpy((file->buf + file->cur), s);
	file->cur += len;
	file->fsnode->length = file->cur;
	file->isdirty = TRUE;
	return (((int)len) > 0) ? ((int)len) : 1;
}

int unlink(const char * pathname)
{
	if(pathname == NULL)
		return -1;

	FS_NODE * fs_root = (FS_NODE *)syscall_get_fs_root();
	FS_NODE * fsnode = (FS_NODE *)syscall_umalloc(sizeof(FS_NODE));
	SINT32 ret_int = (SINT32)syscall_finddir_fs_safe(fs_root, (char *)pathname, fsnode);
	if(ret_int == -1)
	{
		syscall_ufree(fsnode);
		return -1;
	}
	if(syscall_remove_fs(fsnode) == 0)
	{
		syscall_ufree(fsnode);
		return -1;
	}
	else
	{
		syscall_ufree(fsnode);
		return 0;
	}
	return -1;
}

int rename(const char * oldpath, const char * newpath)
{
	if(oldpath == NULL)
		return -1;
	if(newpath == NULL)
		return -1;

	FILE * oldfile = fopen((char *)oldpath, "r");
	if(oldfile == NULL)
		return -1;
	FILE * newfile = fopen((char *)newpath, "w");
	if(newfile == NULL)
	{
		fclose(oldfile);
		return -1;
	}
	syscall_write_fs(newfile->fsnode,0,oldfile->fsnode->length,oldfile->buf);
	newfile->isdirty = FALSE;
	fclose(oldfile);
	fclose(newfile);
	return unlink(oldpath);
}

char * strchr( char *str, char ch )
{
	 if (str == NULL)
	 {
		return NULL;
	 }

	 while (*str != '\0')
	 {
		if (*str == ch)
		{
			return str;
		}
		   
		++str;
	 }

	if(ch == '\0')
		return str;
	else
		return NULL;
}

//大写字母转换为小写字母
int tolower(int ch)
{
   if ( (unsigned int)(ch - 'A') < 26u )
      ch += 'a' - 'A';
   return ch;
}

UINT8 get_control_key()
{
	return (UINT8)syscall_get_control_keys();
}

UINT8 release_control_keys(UINT8 key)
{
	return (UINT8)syscall_release_control_keys(key);
}

int deleteln()
{
	return syscall_monitor_del_line();
}

int insertln()
{
	return syscall_monitor_insert_line();
}

void srand(unsigned int seed)
{
	libc_random_seed = seed;
}

/* This algorithm is mentioned in the ISO C standard, here extended
   for 32 bits. you can see 
   http://stackoverflow.com/questions/1026327/what-common-algorithms-are-used-for-cs-rand
 */
int rand_r(unsigned int *seed)
{
  unsigned int next = *seed;
  int result;

  next *= 1103515245;
  next += 12345;
  result = (unsigned int) (next / 65536) % 2048;

  next *= 1103515245;
  next += 12345;
  result <<= 10;
  result ^= (unsigned int) (next / 65536) % 1024;

  next *= 1103515245;
  next += 12345;
  result <<= 10;
  result ^= (unsigned int) (next / 65536) % 1024;

  *seed = next;

  return result;
}

int rand(void)
{
	return rand_r(&libc_random_seed);
}

char *tmpnam(char *s)
{
	if(s != NULL && (strlen(s) < 19))
		return NULL;

	srand(syscall_timer_get_tick());
	FS_NODE * fs_root = (FS_NODE *)syscall_get_fs_root();
	FS_NODE * fsnode = (FS_NODE *)syscall_umalloc(sizeof(FS_NODE));
	SINT32 ret_int = (SINT32)syscall_finddir_fs_safe(fs_root, "hd", fsnode);
	if(ret_int == -1)
	{
		syscall_ufree(fsnode);
		return NULL;
	}
	int i;
	do
	{
		strcpy(tmpnam_bbuf, "hd/tmp/tmpXXXXXXXXX");
		for(i=18;i>=10;i--)
		{
			if(tmpnam_bbuf[i] == 'X')
				tmpnam_bbuf[i] = tmpnam_random_char[rand() % 18];
		}
		ret_int = (SINT32)syscall_finddir_fs_safe(fs_root, tmpnam_bbuf, fsnode);
		if(ret_int == -1)
			break;
	}while(TRUE);
	syscall_ufree(fsnode);
	if(s != NULL)
	{
		strcpy(s, tmpnam_bbuf);
		return s;
	}
	return tmpnam_bbuf;
}

FILE * tmpfile()
{
	if(libc_tmpfile != NULL)
		return libc_tmpfile;
	char * name = tmpnam(NULL);
	if(name == NULL)
		return NULL;
	libc_tmpfile = fopen(name, "w");
	return libc_tmpfile;
}

void unlink_tmpfile()
{
	if(libc_tmpfile == NULL)
		return ;
	
	syscall_remove_fs(libc_tmpfile->fsnode);
	libc_tmpfile->isdirty = FALSE;
	fclose(libc_tmpfile);
	libc_tmpfile = NULL;
}

int fseek(FILE * file, long offset, int whence)
{
	if(file == NULL)
		return -1;
	if(file->sign != FILE_SIGN)
		return -1;
	if(file->fsnode == NULL)
		return -1;
	if(file->buf == NULL)
		return -1;

	switch(whence)
	{
	case SEEK_SET:
		whence = 0;
		break;
	case SEEK_CUR:
		whence = file->cur;
		break;
	case SEEK_END:
		whence = file->fsnode->length;
		break;
	}

	if((whence + offset) < 0)
		file->cur = 0;
	else if((whence + offset) <= file->fsnode->length)
		file->cur = (UINT32)(whence + offset);
	else
		file->cur = file->fsnode->length;
	return file->cur;
}

int fread(void *ptr, size_t size, size_t nmemb, FILE * file)
{
	if(file == NULL)
		return -1;
	if(file->sign != FILE_SIGN)
		return -1;
	if(file->fsnode == NULL)
		return -1;
	if(file->buf == NULL)
		return -1;
	if(ptr == NULL)
		return -1;

	size_t len = size * nmemb;
	if((file->cur + len) > file->fsnode->length)
		len = file->fsnode->length - file->cur;
	if(len == 0)
		return 0;
	memcpy((UINT8 *)ptr, (UINT8 *)file->buf, len);
	return (((int)len) > 0) ? ((int)len) : 1;
}

int fwrite(const void *ptr, size_t size, size_t nmemb, FILE * file)
{
	if(file == NULL)
		return -1;
	if(file->sign != FILE_SIGN)
		return -1;
	if(file->fsnode == NULL)
		return -1;
	if(file->buf == NULL)
		return -1;
	if(ptr == NULL)
		return -1;

	size_t len = size * nmemb;

	if(len == 0)
		return -1;

	if(file->cur + len > file->buf_size)
	{
		UINT32 ext_size = ((file->cur + len - file->buf_size) / FILE_EXT_BUF_SIZE) + 1;
		ext_size *= FILE_EXT_BUF_SIZE;
		if(file_extend_buf(file, ext_size) == NULL)
			return -1;
	}
	memcpy((UINT8 *)(file->buf + file->cur), (UINT8 *)ptr, len);
	file->cur += len;
	file->fsnode->length = file->cur;
	file->isdirty = TRUE;
	return (((int)len) > 0) ? ((int)len) : 1;
}

void exit(int code)
{
	syscall_exit(code);
}

