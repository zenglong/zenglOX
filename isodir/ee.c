#include <stdlib.h>

#define EE_MAJOR_VERSION 1
#define EE_MINOR_VERSION 0
#define EE_REVISION 0

#define EOL '\0' /* end of line marker */
#define BLK ' ' /* blank */
#define LF  '\n' /* new line */
#define NLEN  80 /* buffer line for file name */
#define LMAX  10000 /* max line length */
#define XINC  20    /* increament of x */
#define HLP 28

#define CHG 0
#define FIL 1   /* fill */
#define OVR 2   /* insert */
#define CAS 3   /* case sensative */
#define TAB 4   /* tab */
#define POS 5   /* show pos */
#define ALT 6   /* meta_key */
#define NEW 7   /* new file */
#define SHW 8   /* screen */
#define EDT 9   /* quit edit */
#define WIN 10  /* window */
#define NTS 11  /* note posted */
#define ALL 12  /* last flag, quit */
char flag[ALL+1] = {0};
char fsym[]="|foctp~*";

typedef struct {
	char name[NLEN];
	int  jump;
} MWIN; /* my window structure */

/* order of \n\r is important for curses */
#define HELP_STR "\n\
Usage: ee [-+line(def=1) -ttab(def=8) file(def=hd/Notes)]\n\r\
\n\
ctrl+a: goto file bottom    ctrl+shift+a: goto file top\n\r\
ctrl+b: set block mark      ctrl+c: block copy\n\r\
ctrl+d: block delete        ctrl+e: case sensative\n\r\
ctrl+f: forward search      ctrl+shift+f: backward search\n\r\
ctrl+g: goto line           ctrl+i: show status\n\r\
ctrl+l: goto col            ctrl+m: line format\n\r\
ctrl+q: forward tab move    ctrl+r: replace\n\r\
ctrl+shift+r: replace all   ctrl+s: save file\n\r\
ctrl+t: switch tab mode     ctrl+v: block paste\n\r\
ctrl+w: backward tab move   ctrl+x: block cut\n\r\
ctrl+z: exit                ctrl+Backspace: backspace and delete tab\n\r\
F1 key: show help           F2 key: switch line col show\n\r\
Home key: goto line begin   End key: goto line end\n\r\
Left key: cursor left       Right key: cursor right\n\r\
Up key: cursor up           Down key: cursor down\n\r\
Page Up key: page up        Page Down key: page down\n\r\
Backspace key: backspace    Enter key: newline\n\r\
ESC+z: exit\n\r\
please note that you must use left ctrl key , not right ctrl!\n\r\
and the letter is all lower case letter!\n\r\
Press any key to continue ..."

#define AMAX 0xD0000 /* main buffer size */
#define BMAX 0x400 /* block size */
#define YTOP 0 /* first line */

#define ttopen() initscr(); sww=76; swhfull=23
#define ttclose() endwin()
#define gotoxy(x,y) move(y,x-1)
#define insline() insertln()
#define delline() deleteln()
#define clreol() cputs_line(" ")

char  bbuf[20] = {0}; /* backup file name */
char  sbuf[NLEN], rbuf[NLEN]; /* search buffer, replace buffer */
char  *ae, aa[AMAX]; /* main buffer */
char  bb[BMAX], *mk; /* block buffer, mark */
int blen = 0;
char  *dp, *ewb; /* current pos, line */

int xtru, ytru; /* file position */
int ytot; /* 0 <= ytru <= ytot */

int swhfull; /* screen physical height */
int x, sww; /* screen position 1 <= x <= sww */
int y, swh; /* screen size 0 <= y <= swh */
int y1, y2; /* 1st, 2nd line of window */
int tabsize=8; /* tab size */

FILE  *fi = 0, *fo = 0;
MWIN  win, winnext, wincopy; /* current, next, other windows */

void cursor_up(void), cursor_down(void), cursor_left(void), cursor_right(void);
void show_rest(int len, char *s);
void show_scr(int fr, int to);
void show_sup(int line), show_sdn(int line);
void show_flag(int x, int g);
void show_note(char *prp);
void show_top(void);
void file_read(void);
void file_save(int f_all, int f_win);
void file_rs(char *s, char *d);
void goto_x(int xx), goto_y(int yy);

/* honest get a key */
int get_key()
{
	int key;
get_key_start:
	key = getch();
	if(key == 127) 
		return 8;
	switch(key)
	{
	case MKK_CURSOR_UP_PRESS:
		return 'E' & 0x1F;
		break;
	case MKK_CURSOR_DOWN_PRESS:
		return 'X' & 0x1F;
		break;
	case MKK_CURSOR_RIGHT_PRESS:
		return 'D' & 0x1F;
		break;
	case MKK_CURSOR_LEFT_PRESS:
		return 'S' & 0x1F;
		break;
	case MKK_PAGE_UP_PRESS:
		return 'V' & 0x1F;
		break;
	case MKK_PAGE_DOWN_PRESS:
		return 'W' & 0x1F;
		break;
	case MKK_INSERT_PRESS:
		return 'N' & 0x1F;
		break;
	case MKK_DELETE_PRESS:
		return 'M' & 0x1F;
		break;
	case MKK_HOME_PRESS:
		return 'F' & 0x1F;
		break;
	case MKK_END_PRESS:
		return 'G' & 0x1F;
		break;
	case MKK_F1_PRESS:
		return 'L' & 0x1F;
		break;
	case MKK_F2_PRESS:
		return 'U' & 0x1F;
		break;
	case MKK_F3_PRESS:
	case MKK_F4_PRESS:
	case MKK_F5_PRESS:
	case MKK_F6_PRESS:
	case MKK_F7_PRESS:
	case MKK_F8_PRESS:
	case MKK_F9_PRESS:
	case MKK_F10_PRESS:
		goto get_key_start;
	}
	if(key == 27) { // ESC
		flag[ALT]++;
		show_flag(ALT, flag[ALT]);
		key = getch();
		return key;
	}
	return key;
}

/* cursor movement ----------------------------------------- */
void cursor_up(void)
{
	if(ytru == 0) 
		return;
	ytru--;
	while(*--ewb != EOL) 
		;
	y--;
}

void cursor_down(void)
{
	if(ytru == ytot)
		return;
	ytru++;
	while(*++ewb != EOL) 
		;
	y++;
}

/* cursor left & right: x, xtru */
void cursor_left(void)
{
	if(xtru == 1) 
		return;
	xtru--;
	if(--x < 1) {
		x += XINC;
		flag[SHW]++;
	}
}

void cursor_right(void)
{
	if(xtru == LMAX) 
		return;
	xtru++;
	if(++x > sww) {
		x -= XINC;
		flag[SHW]++;
	}
}

#define cursor_pageup() {int i; for(i=1; i<swh; ++i) cursor_up();}
#define cursor_pagedown(){int i; for(i=1; i<swh; ++i) cursor_down();}

/* dispaly --------------------------------------------------------*/
/* assuming cursor in correct position: show_rest(sww-x,ewb+xtru) */
void show_rest(int len, char * s)
{
	char save;
	save = s[len];
	s[len] = 0;
	cputs_line(s);
	s[len] = save;
}

/* ewb and y correct */
void show_scr(int fr,int to)
{
	char *s=ewb;
	int  len=sww-1, i=fr;
	unsigned xl=xtru-x;

	/* calculate s */
	for(; i<y; i++) 
		while(*--s != EOL) 
			;
	for(; i>y; i--) 
		while(*++s != EOL) 
			;

	/* first line */
	s++;
	do {
		gotoxy(1, fr+y2);
		if(s<ae && strlen(s) > xl) 
			show_rest(len, s+xl);
		else
			clreol();
		while(*s++) 
			;
	} while(++fr <= to);
}

void show_sup(int line)
{
	gotoxy(1, y2+line);
	delline();
	show_scr(swh, swh);
}

void show_sdn(int line)
{
	gotoxy(1, y2+line);
	insline();
	show_scr(line, line);
}

void show_flag(int x, int g)
{
	gotoxy(14+x, y1);
	putch(g ? fsym[x]: '.');
	flag[x] = g;
}

void show_note(char * prp)
{
	gotoxy(17+ALT, y1);
	cputs(prp);
	clreol();
	flag[NTS]++;
}

int show_gets(prp, buf)
char *prp, *buf;
{
	int key;
	int col = strlen(buf);
	show_note(prp);
	cputs(": ");
	cputs(buf);
	clreol();
	for(;;) {
		key = get_key();
		if(key >= BLK) {
			if(col < 0) {
				col++;
				show_note(prp);
				cputs(": ");
			}
			buf[col++] = key;
		}
		else if(key == 8) {
			if(col < 0) 
				col = strlen(buf);
			if(col == 0) 
				continue;
			col--;
		}
		else 
			break;
		putch(key);
		clreol();
	}
	flag[ALT] = 0;
	if(col > 0) 
	{
		buf[col] = 0;
	}
	flag[SHW]++;
	return (key == 27 || *buf == 0);
}

void show_top(void)
{
	int i;
	gotoxy(1, y1);
	cputs_line(win.name);
	for(i=0; i<=NEW; i++)
		show_flag(i, flag[i]);
	flag[NTS]++;
}

void show_help(void)
{
	clrscr();
	cputs(HELP_STR);
	get_key();
	show_top();
	flag[SHW]++;
}

void show_status(void)
{
	char tbuf[160];
	sprintf(tbuf, "line %d/%d, col %d, char %u/%u/%u+%u press any key to continue",
		ytru+1, ytot+1, xtru, dp-aa, ae-aa, BMAX, AMAX);
	show_note(tbuf);
	get_key();
	flag[SHW]++;
}

/* file operation ---*/
void file_read(void)
{
	int  c;
	char *col;
	ewb = aa;
	ae = mk = col = aa+1;
	xtru = x = 1;
	ytru = y = ytot = 0;
	if(fi == 0) return;

	/* read complete line */
	do {
		c = fgetc(fi);
		if(c == EOF) 
		{
			fclose(fi);
			fi = 0;   /* no more read */
			break;
		}
		if(c == 9) 
		{
			/* tab */
			if(flag[TAB] == 0) show_flag(TAB, 1);
			do (*ae++ = BLK);
			while( ((ae-col) % tabsize) != 0);
		}
		else if(c == LF) 
		{
			*ae++ = EOL;
			col = ae;
			ytot++;
		}
		else 
			*ae++ = c;
	} while(ae < aa+AMAX-BMAX || c != LF);

	for(; win.jump>1; win.jump--) 
		cursor_down();
}

/* compress one line from end */
char *file_ltab(s)
char *s;
{
	char *d, *e;
	e = d = strchr(s--, EOL);
	//while(*--e == BLK) 
	//	;  /* trailing blank */
	--e;
	while(e > s) 
	{
		if(e[0] == BLK && (e-s)%tabsize == 0 && e[-1] == BLK) 
		{
			*--d = 9;
			while(*--e == BLK && (e-s)%tabsize != 0) 
				;
		}
		else 
			*--d = *e--;
	}
	return d;
}

/* routine used for write block file also, this makes it more complicated */
int file_write(fp, s, e)
FILE *fp;
char *s, *e;
{
	if(fp == 0) 
		return 1; /* no write */
	do {
		if(flag[TAB] && *s != EOL) 
			s = file_ltab(s);
		/* if s="", TC return 0, TC++ return -1 */
		if(*s && fputs(s, fp) <= 0) 
			return 1;
		fputc(LF, fp);
		while(*s++ != EOL) 
			;
	} while(s < e);
	return 0;
}

int file_fout(void)
{
	if(fo == 0) {
		strcpy(bbuf, "hd/tmp/tmpXXXXXXXXX");
		fo=fopen((char *)tmpnam(bbuf), "w");
	}
	return file_write(fo, aa+1, ae);
}

void file_save(int f_all, int f_win)
{
	int k='n';
	if(flag[CHG])
		do {
			show_note("Save file (yes/no/cancel): ");
			k = tolower(get_key());
			if(k == 'c') 
				return;
		} while(k != 'y' && k != 'n');

	flag[CHG] = 0;
	flag[EDT]++;
	flag[ALL] = f_all;
	flag[WIN] = f_win;
	if(k == 'n') {
		if(fo ) {
			fclose(fo);
			unlink(bbuf);
			fo = 0;
		}
		return;
	}
	if(file_fout() ) 
		return; /* no write */
	while(fi != 0 ) {
		file_read();
		file_fout();
	}
	fclose(fo);
#if !VMS
	if(flag[NEW] == 0) 
		unlink(win.name);
#endif
	rename(bbuf, win.name);
	fo = 0;
}

void saved_file()
{
	int k='n';
	if(flag[CHG])
	{
		do {
			show_note("Save file (yes/no): ");
			k = tolower(get_key());
			if(k == 'n') 
				return;
		} while(k != 'y');
	}
	else
		return;

	flag[CHG] = 0;
	show_flag(CHG, flag[CHG]);
	if(file_fout() ) 
		return; /* no write */
	while(fi != 0 ) {
		file_read();
		file_fout();
	}
	fclose(fo);
#if !VMS
	if(flag[NEW] == 0) 
		unlink(win.name);
#endif
	rename(bbuf, win.name);
	fo = 0;
	show_note("saved file , press any key to continue.");
	get_key();
}

void file_rs(char * s, char * d)
{
	char *e = ae;
	unsigned i = e-s;

	/* immediate problem only when block buffer on disk too big */
	if((ae += (d-s)) >= aa+AMAX) {
		show_note("Main buffer full, press any key to exit");
		get_key();
		exit(-1);
		return;
	}
	if(s < d) { /* expand */
		d += e - s;
		s = e;
		while(i-- > 0) 
			*--d = *--s;
		/* while(j-- > 0) if((*--d = *--str) == EOL) ytot++; */
	}
	else {
		/* adjust ytot when shrink */
		for(e=d; e<s; e++) 
			if(*e == EOL) 
				ytot--;
		while(i-- > 0) 
			*d++ = *s++;
	}
	*ae = EOL;  /* last line may not complete */
	if(!flag[CHG] ) {
		show_flag(CHG, 1);
		gotoxy(x, y+y2);
	}
}

/* search and goto */
/* xx >= 1, yy >= 0 */
void goto_x(int xx)
{
	int i, n;
	n = xtru;
	for(i=xx; i<n; i++) 
		cursor_left();
	for(i=n; i<xx; i++) 
		cursor_right();
}

void goto_y(int yy)
{
	int i, n;
	n = ytru;
	for(i=yy; i<n; i++) 
		cursor_up();
	for(i=n; i<yy; i++) 
		cursor_down();
}

void goto_ptr(s)
char *s;
{
	/* find ewb <= s */
	char  *s1 = s;
	while(*--s1 != EOL) 
		;
	while(ewb > s) 
		cursor_up();
	while(ewb < s1) 
		cursor_down();
	goto_x(s-ewb);
	if(y > swh) 
		y = flag[SHW] = swh/4;
}

void goto_row(void)
{
	static char rbuf[6];
	show_gets("Goto line", rbuf);
	goto_y(atoi(rbuf)-1);
}

void goto_col(void)
{
	static char cbuf[6];
	show_gets("Goto Column", cbuf);
	goto_x(atoi(cbuf) );
}

/* compare to sbuf. used by search */
int str_cmp(s)
char *s;
{
	char  *d = sbuf;
	if(flag[CAS] ) {
		while(*d ) 
			if(*s++ != *d++ ) 
				return 1;
		return 0;
	}
	while(*d ) 
		if(tolower(*s++) != tolower(*d++)) 
			return 1;
	return 0;
}

/* back / forward search */
char *goto_find(s, back)
char *s;
int  back;
{
	do {
		if(back ) {
			if(--s <= aa) 
				return 0;
		}
		else if(++s >= ae) 
			return 0;
	} while(str_cmp(s));
	goto_ptr(s);
	return s;
}

void goto_search(back)
int  back;
{
	int ret=0;
	if(back)
		ret = show_gets("Search back for", sbuf);
	else
		ret = show_gets("Search forward for", sbuf);
	release_control_keys(LIBC_CK_ALT);
	if(ret) 
		return;
	if(goto_find(dp, back) == 0)
	{
		show_note("Not Found, press any key to continue");
		get_key();
	}
}

void goto_replace(whole)
int whole;
{
	char  *s=dp;
	int rlen, slen = strlen(sbuf);
	if(str_cmp(s))
	{
		show_note("Invalid Pos, must search first.");
		get_key();
		return;
	}
	if((!whole ? show_gets("Replace with", rbuf) : show_gets("Replace whole with", rbuf))) 
		return;
	release_control_keys(LIBC_CK_ALT);
	rlen = strlen(rbuf);
	do {
		file_rs(s+slen, s+rlen);
		memcpy(s, rbuf, rlen);
	}while(whole && (s=goto_find(s, 0)) != 0);
	if(whole) 
		flag[SHW]++;
	else {
		gotoxy(x, y+y2);
		show_rest(sww-x, s);
	}
}

/* block and format ---*/
/* use blen, mk, bb */
FILE *fb = 0;

void block_put(void)
{
	if(blen <= 0)
	{
		show_note("Invalid Pos, press any key to continue");
		get_key();
		return;
	}
	if(blen < BMAX) 
		memcpy(bb, mk, blen);
	else {
		if(fb == 0 && (fb = tmpfile()) == 0) 
			return;
		fseek(fb, 0L, 0);
		fwrite(mk, 1, blen, fb);
	}
}

void block_get(void)
{
	int i;
	if(blen < BMAX && blen > 0) 
		memcpy(mk, bb, blen);
	else {
		if(fb == 0) 
			return;
		fseek(fb, 0L, 0);
		fread(mk, 1, blen, fb);
	}
	/* calculate ytot */
	for(i=0; i<blen; i++) 
		if(mk[i] == EOL) 
			ytot++;
}

void block_mark(void)
{
	if(*dp == EOL)
	{
		show_note("Invalid Pos, press any key to continue");
		get_key();
	}
	else {
		mk = dp;
		show_note("Mark Set, press any key to continue");
		get_key();
	}
}

void block_copy(delete)
int delete;
{
	if(delete)
	{
		int k='n';
		do {
			show_note("cut the block (yes/no): ");
			k = tolower(get_key());
			if(k == 'n') 
				return;
		} while(k != 'y');
	}
	blen = dp - mk;
	if(blen <= 0)
	{
		show_note("Invalid Pos, press any key to continue");
		get_key();
		return;
	}
	block_put();
	if(delete) {
		goto_ptr(mk);
		file_rs(dp, mk);
		flag[SHW]++;
	}
	if(!delete)
	{
		show_note("Block copied, press any key to continue");
		get_key();
	}
}

void block_delete()
{
	int k='n';
	do {
		show_note("delete the block (yes/no): ");
		k = tolower(get_key());
		if(k == 'n') 
			return;
	} while(k != 'y');
	blen = dp - mk;
	if(blen <= 0)
	{
		show_note("Invalid Pos, press any key to continue");
		get_key();
		return;
	}
	goto_ptr(mk);
	file_rs(dp, mk);
	flag[SHW]++;
}

void block_paste(void)
{
	mk = dp;
	file_rs(mk, mk+blen);
	block_get();    /* disk file ??? */
	/* if it is a line */
	if(xtru == 1 && strlen(mk) == blen-1) 
		show_sdn(y);
	else {
		show_scr(y, swh);
		goto_ptr(mk+blen);
	}
}

/* fill current line*/
int line_fill(void)
{
	int i=sww;
	i = sww-1;
	file_rs(ewb+i, ewb+i+1);
	ewb[i] = EOL;
	ytot++;
	cursor_down();
	return i;
}

/* line format */
void line_format(void)
{
	char  *s=ewb;
	int ytmp = y;
	goto_x(1);
	while(strlen(ewb+1) >= sww) 
		line_fill();
	while(ewb[xtru] ) 
		cursor_right();
	if( flag[SHW] == 0) 
		show_scr(ytmp, swh);
	goto_x(strlen(ewb+1)+1);
}

/* key actions ---*/
/* update file part, then screen ... */
void key_return(void)
{
	char  *s=dp;
	if(flag[OVR] ) {
		cursor_down();
		goto_x(1);
		return;
	}
	file_rs(s, s+1);
	goto_x(1);
	*s = EOL;
	ytot++;
	cursor_down();
	if(flag[SHW] == 0) {
		clreol();
		if(y < sww) 
			show_sdn(y);
	}
}

/* used by next two */
void key_deleol(char * s)
{
	if(ytru == ytot) 
		return;
	goto_x(s-ewb);
	file_rs(s+1, s);
	if(flag[SHW] ) 
		return;
	if(y < 0) {   
		/* y = -1 */
		y = 0;
		show_scr(0,0);
	}
	else {
		gotoxy(x, y+y2);
		show_rest(sww-x, s);
		show_sup(y+1);
	}
}

/* delete under */
void key_delete(void)
{
	char  *s=dp;
	if( *s == EOL) {
		key_deleol(s);
		return;
	}
	file_rs(s+1, s);
	show_rest(sww-x, s);
}

void key_backspace(BOOL deltab)
{
	char  *s=dp;
	if(*--s == EOL) { 
		/* delete EOL */
		if(ytru == 0) 
			return;
		cursor_up();
		key_deleol(s);
		return;
	}
	while(ewb+xtru > s) 
		cursor_left();
	/* delete tab space */
	if(deltab)
	{
		if(*s == BLK) {
			while(*--s == BLK && (xtru%tabsize) != 1) 
				cursor_left();
			s++;
		}
	}
	file_rs(dp, s);
	if(!flag[SHW] ) {
		gotoxy(x, y+y2);
		show_rest(sww-x, s);
	}
}

void key_tab(tabi)
int tabi;
{
	char  *s = ewb+xtru;
	int xtmp=x;
	do 
		cursor_right();
	while((xtru%tabsize) != 1);
	if(!tabi && s==dp) {
		s = ewb+xtru;
		file_rs(dp, s); /* may change cursor_position */
		while(s > dp) 
			*--s = BLK;
		gotoxy(xtmp, y+y2);
		show_rest(sww-xtmp, s);
	}
}

void key_left_tab(void)
{
	do 
		cursor_left();
	while((xtru%tabsize) != 1);
}

void key_normal(int key)
{
	char * s = ewb+xtru;
	int xtmp;
	if(dp < s) {
		file_rs(dp, s);
		while(dp < s) 
			*dp++ = BLK;
	}
	if(flag[OVR] && *s != EOL) {
		putch(*s = key);
		flag[CHG] = 1;
		show_flag(CHG, 1);
	}
	else {
		file_rs(s, ++dp);
		*s = key;
		show_rest(sww-x, s);
	}
	cursor_right();
	if(!flag[FIL] || xtru<sww) 
		return;

	// wait for write
}

/* main function */
/*void main_meta_alt(int key)
{
	switch(key) {
	case 'z': 
		file_save(1, 0); 
		break;
	case 'q':
		key_tab(1);
		break;
	case 'w':
		key_left_tab();
		break;
	case 'f':
		show_status();
		break;
	case 'g':
		goto_row();
		break;
	case 'l':
		goto_col();
		break;
	case 's':
		goto_search(0);
		break;
	case 'd':
		goto_search(1);
		break;
	case 'r':
		goto_replace(0);
		break;
	case 'R':
		goto_replace(1);
		break;
	case 'c':
		show_flag(CAS, !flag[CAS]);
		break;
	}

	//release_control_keys(LIBC_CK_ALT);
	// wait for write
	show_flag(ALT, 0);
}*/

void main_meta_ctrl(int key)
{
	switch(key) {
	case 'a':goto_y(ytot);break;
	case 'A':goto_y(0);break;
	case 'b':block_mark();break;
	case 'c':block_copy(0);break;
	case 'd':block_delete();break;
	case 'e':show_flag(CAS, !flag[CAS]);break;
	case 'f':goto_search(0);break;
	case 'F':goto_search(1);break;
	case 'g':goto_row();break;
	case 'i':show_status();break;
	case 'l':goto_col();break;
	case 'm':line_format();break;
	case 'q':key_tab(1);break;
	case 'r':goto_replace(0);break;
	case 'R':goto_replace(1);break;
	case 's':saved_file();break;
	case 't':show_flag(TAB, !flag[TAB]);break;
	case 'v':block_paste();break;
	case 'w':key_left_tab();break;
	case 'x':block_copy(1);break;
	case 'z':file_save(1, 0); break;
	case 8:
		key_backspace(TRUE);
		break;
	}
}

void main_exec(int key)
{
	int i = xtru;
	dp = ewb;
	while(*++dp && --i>0) 
		;

	UINT8 control_key = get_control_key();

	if(flag[ALT])
	{
		if(key == 'z') 
		{
			file_save(1, 0);
		}
		flag[ALT] = 0;
		show_flag(ALT, flag[ALT]);
	}
	//else if(control_key & LIBC_CK_ALT)
	//	main_meta_alt(key);
	else if(control_key & LIBC_CK_CTRL)
		main_meta_ctrl(key);
	else if(key >= BLK)
		key_normal(key);
	else switch(key | 0x60) {
	case 'e':
		cursor_up();
		break;
	case 'x':
		cursor_down();
		break;
	case 'd':
		cursor_right();
		break;
	case 's':
		cursor_left();
		break;
	case 'v':
		cursor_pageup();
		break;
	case 'w':
		cursor_pagedown();
		break;
	case 'h':
		key_backspace(FALSE);
		break;
	case 'i': 
		key_tab(0); 
		break;
	case 'j': 
		key_return(); 
		break;
	case 'n':
		show_flag(OVR, !flag[OVR]);
		break;
	case 'm':
		key_delete();
		break;
	case 'f': // home press
		goto_x(1);
		break;
	case 'g': // end press
		goto_x(strlen(ewb+1)+1);
		break;
	case 'l': // press F1 key to show help info
		show_help();
		break;
	case 'u': // press F2 key to show line col number
		show_flag(POS, !flag[POS]);
		flag[NTS]++;
		break;
	}
}

/* win is only thing it knows */
void main_loop(void)
{
	int  yold, xold;

	show_top();
	if((fi = fopen(win.name, "r")) == NULL) {
		//show_note("New file");
		flag[NEW]++;
    		show_flag(NEW, flag[NEW]);
	}
	file_read();

	while(flag[EDT] == 0) {
		if(y <= -1 || y >= swh) {      /* change here if no hardware scroll */
			if(y == -1) {
				y++; show_sdn(0);
			}
			else if(y == swh) {
				y--; show_sup(0);
			}
			else {
				y = y < 0? 0: swh-1;
				show_scr(0, swh);
				flag[SHW] = 0;
			}
		}
		else if(flag[SHW] ) {
			show_scr(0, swh);
			flag[SHW] = 0;
		}
		if(flag[NTS] ) {
			show_note(flag[POS] ? "Line      Col       F1-Help  F2-switch line col top show" : "F1-Help  F2-switch line col top show");
			yold = xold = (-1);
			flag[NTS] = 0;
		}
		if(flag[POS] ) {
			if(ytru != yold) {
				yold = ytru;
				gotoxy(22+ALT, y1);
				cprintf("%-5d", ytru+1);
			}
			if(xtru != xold) {
				xold = xtru;
				gotoxy(31+ALT, y1);
				cprintf("%-4d", xtru);
			}
		}
		gotoxy(x, y+y2);
		main_exec(get_key());
	}
}

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);

	if((argc == 2) && (strcmp(argv[1],"-v")==0))
	{
		printf("ee(easy editor) for zenglOX, ee's version is v%d.%d.%d", EE_MAJOR_VERSION,
					EE_MINOR_VERSION, EE_REVISION);
		return 0;
	}

	/* set command line */
	while(--argc && **++argv == '-')
	{
		switch(*++*argv) {
			case 't': tabsize = atoi(++*argv); break;
			case '+': win.jump = atoi(++*argv); break;
		}
	}

	strcpy(win.name, argc == 0? "hd/Notes" : *argv);
	aa[0] = EOL;

	ttopen();
	swh = swhfull;
	y1 = YTOP;

	flag[POS] = !flag[POS];

	do {
		y2 = y1+1;
		flag[SHW]++;
		flag[NEW] = flag[EDT] = flag[ALL] = 0;
		main_loop();
		// wait for write
		flag[WIN] = 0;
	} while(flag[ALL] == 0) ;

	if(fb != 0)
	{
		unlink_tmpfile();
		fb = 0;
	}
	gotoxy(1, swhfull+1);
	ttclose();
}

