/*zlox_monitor.c Define some functions for writing to the monitor*/

#include "zlox_monitor.h"
#include "zlox_vga.h"

#define ZLOX_MONITOR_COLOR 0xFFFFFFFF
#define ZLOX_MONITOR_BACK_COLOR 0x0
#define ZLOX_MONITOR_W_SPACE 0
#define ZLOX_MONITOR_H_SPACE 2

extern ZLOX_UINT32 vga_current_mode;

// The VGA framebuffer starts at 0xB8000.
ZLOX_UINT16 * video_memory = (ZLOX_UINT16 *)0xB8000;
// Stores the cursor position.
ZLOX_UINT8 cursor_x = 0;
ZLOX_UINT8 cursor_y = 0;
ZLOX_BOOL single_line_out = ZLOX_FALSE;
ZLOX_UINT32 monitor_color = ZLOX_MONITOR_COLOR;
ZLOX_UINT32 monitor_backColour = ZLOX_MONITOR_BACK_COLOR;
ZLOX_SINT32 monitor_w_space = ZLOX_MONITOR_W_SPACE;
ZLOX_SINT32 monitor_h_space = ZLOX_MONITOR_H_SPACE;

// Updates the hardware cursor.
static ZLOX_VOID zlox_move_cursor()
{
	// The screen is 80 characters wide...
	ZLOX_UINT16 cursorLocation = cursor_y * 80 + cursor_x;
	//Tell the VGA board we are setting the high cursor byte.
	zlox_outb(0x3D4, 14); 
	// Send the high cursor byte.
	zlox_outb(0x3D5, cursorLocation >> 8);
	// Tell the VGA board we are setting the low cursor byte.
	zlox_outb(0x3D4, 15);
	// Send the low cursor byte.
	zlox_outb(0x3D5, cursorLocation);
}

// Scrolls the text on the screen up by one line.
static ZLOX_VOID zlox_scroll()
{

	// Get a space character with the default colour attributes.
	ZLOX_UINT8 attributeByte = (0 /*black*/ << 4) | (15 /*white*/ & 0x0F);
	ZLOX_UINT16 blank = 0x20 /* space */ | (attributeByte << 8);

	// Row 25 is the end, this means we need to scroll up
	if(cursor_y >= 25)
	{
		// Move the current text chunk that makes up the screen
		// back in the buffer by a line
		ZLOX_SINT32 i;
		/*for (i = 0*80; i < 24*80; i++)
		{
			video_memory[i] = video_memory[i+80];
		}*/
		zlox_memcpy((ZLOX_UINT8 *)video_memory,(ZLOX_UINT8 *)(video_memory + 80),24*80*2);

		// The last line should now be blank. Do this by writing
		// 80 spaces to it.
		for (i = 24*80; i < 25*80; i++)
		{
			video_memory[i] = blank;
		}
		// The cursor should now be on the last line.
		cursor_y = 24;
	}
}

// 删除当前光标所在的行
ZLOX_SINT32 zlox_monitor_del_line()
{
	// Get a space character with the default colour attributes.
	ZLOX_UINT8 attributeByte = (0 /*black*/ << 4) | (15 /*white*/ & 0x0F);
	ZLOX_UINT16 blank = 0x20 /* space */ | (attributeByte << 8);

	ZLOX_UINT16 * d = video_memory + cursor_y * 80;
	ZLOX_UINT16 * s = cursor_y < 24 ? (d + 80) : (d);
	ZLOX_UINT32 bytes = (24 - cursor_y) * 80 * 2;
	if(cursor_y < 24)
	{
		zlox_memcpy((ZLOX_UINT8 *)d, (ZLOX_UINT8 *)s, bytes);
	}
	ZLOX_SINT32 i;
	// The last line should now be blank. Do this by writing
	// 80 spaces to it.
	for (i = 24*80; i < 25*80; i++)
	{
		video_memory[i] = blank;
	}
	return 0;
}

// 在当前光标所在的行之前插入一个新行
ZLOX_SINT32 zlox_monitor_insert_line()
{
	// Get a space character with the default colour attributes.
	ZLOX_UINT8 attributeByte = (0 /*black*/ << 4) | (15 /*white*/ & 0x0F);
	ZLOX_UINT16 blank = 0x20 /* space */ | (attributeByte << 8);

	ZLOX_UINT16 * s_start = video_memory + cursor_y * 80;
	ZLOX_UINT16 * d_start = cursor_y < 24 ? (s_start + 80) : (s_start);
	ZLOX_UINT32 bytes = (24 - cursor_y) * 80 * 2;
	ZLOX_UINT8 * s_end = (bytes != 0) ? ((ZLOX_UINT8 *)s_start + bytes - 1) : ((ZLOX_UINT8 *)s_start + 80 * 2 - 1);
	ZLOX_UINT8 * d_end = (bytes != 0) ? ((ZLOX_UINT8 *)d_start + bytes - 1) : s_end;
	if(cursor_y < 24)
	{
		zlox_reverse_memcpy(d_end, s_end, bytes);
	}
	ZLOX_SINT32 i;
	for(i = 0; i < 80 ;i++)
	{
		s_start[i] = blank;
	}
	return 0;
}

// Writes a single character out to the screen.
ZLOX_VOID zlox_monitor_put_orig(ZLOX_CHAR c)
{
	// The background colour is black (0), the foreground is white (15).
	ZLOX_UINT8 backColour = 0;
	ZLOX_UINT8 foreColour = 15;

	// The attribute byte is made up of two nibbles - the lower being the 
	// foreground colour, and the upper the background colour.
	ZLOX_UINT8  attributeByte = (backColour << 4) | (foreColour & 0x0F);
	// The attribute byte is the top 8 bits of the word we have to send to the
	// VGA board.
	ZLOX_UINT16 attribute = attributeByte << 8;
	ZLOX_UINT16 *location;

	// Handle a backspace, by moving the cursor back one space
	if (c == 0x08)
	{
		if(cursor_x)
			cursor_x--;
		else if(cursor_x == 0)
		{
			if(cursor_y != 0)
			{
				cursor_y = (cursor_y - 1);
				cursor_x = 79;
			}
		}
	}

	// Handle a tab by increasing the cursor's X, but only to a point
	// where it is divisible by 8.
	else if (c == 0x09)
	{
		cursor_x = (cursor_x+8) & ~(8-1);
	}

	// Handle carriage return
	else if (c == '\r')
	{
		cursor_x = 0;
	}

	// Handle newline by moving cursor back to left and increasing the row
	else if (c == '\n')
	{
		cursor_x = 0;
		cursor_y++;
	}
	// Handle any other printable character.
	else if(c >= ' ')
	{
		location = video_memory + (cursor_y*80 + cursor_x);
		*location = c | attribute;
		cursor_x++;
	}

	// Check if we need to insert a new line because we have reached the end
	// of the screen.
	if (cursor_x >= 80)
	{
		cursor_x = 0;
		cursor_y ++;
	}

	if(!single_line_out)
	{
		// Scroll the screen if needed.
		zlox_scroll();
		// Move the hardware cursor.
		zlox_move_cursor();
	}
}

// Writes a single character out to the screen.
ZLOX_VOID zlox_monitor_put(ZLOX_CHAR c)
{
	if(vga_current_mode != ZLOX_VGA_MODE_VBE_1024X768X32)
		return;

	ZLOX_SINT32 start_x = 10;
	ZLOX_SINT32 start_y = 10;
	ZLOX_SINT32 w_space = monitor_w_space;
	ZLOX_SINT32 h_space = monitor_h_space;
	ZLOX_UINT32 color = monitor_color;
	ZLOX_UINT32 backColour = monitor_backColour;
	ZLOX_SINT32 val = (ZLOX_SINT32)c;
	if(val < 0)
		val = 0;

	// Handle a backspace, by moving the cursor back one space
	if (c == 0x08)
	{
		if(cursor_x)
			cursor_x--;
		else if(cursor_x == 0)
		{
			if(cursor_y != 0)
			{
				cursor_y = (cursor_y - 1);
				cursor_x = 79;
			}
		}
	}
	// Handle a tab by increasing the cursor's X, but only to a point
	// where it is divisible by 8.
	else if (c == 0x09)
	{
		cursor_x = (cursor_x+8) & ~(8-1);
	}

	// Handle carriage return
	else if (c == '\r')
	{
		cursor_x = 0;
	}

	// Handle newline by moving cursor back to left and increasing the row
	else if (c == '\n')
	{
		cursor_x = 0;
		cursor_y++;
	}
	// Handle any other printable character.
	else if(c >= ' ')
	{
		ZLOX_SINT32 x = start_x + cursor_x * (ZLOX_VGA_CHAR_WIDTH + w_space);
		ZLOX_SINT32 y = start_y + cursor_y * (ZLOX_VGA_CHAR_HEIGHT + h_space);
		zlox_vga_write_char(x, y, val, color, backColour);
		cursor_x++;
	}

	// Check if we need to insert a new line because we have reached the end
	// of the screen.
	if (cursor_x >= 80)
	{
		cursor_x = 0;
		cursor_y ++;
	}

	if(cursor_y >= 36)
	{
		cursor_y = 12;
	}
}

ZLOX_VOID zlox_monitor_set_single(ZLOX_BOOL flag)
{
	single_line_out = flag;
}

ZLOX_VOID zlox_monitor_set_cursor(ZLOX_UINT8 x, ZLOX_UINT8 y)
{
	cursor_x = x;
	cursor_y = y;
	// Move the hardware cursor.
	zlox_move_cursor();
}

// Clears the screen, by copying lots of spaces to the framebuffer.
ZLOX_VOID zlox_monitor_clear()
{
	// Make an attribute byte for the default colours
	ZLOX_UINT8 attributeByte = (0 /*black*/ << 4) | (15 /*white*/ & 0x0F);
	ZLOX_UINT16 blank = 0x20 /* space */ | (attributeByte << 8);

	ZLOX_SINT32 i;
	for (i = 0; i < 80*25; i++)
	{
		video_memory[i] = blank;
	}

	// Move the hardware cursor back to the start.
	cursor_x = 0;
	cursor_y = 0;
	zlox_move_cursor();
}

// Outputs a null-terminated ASCII string to the monitor.
ZLOX_VOID zlox_monitor_write(const ZLOX_CHAR * c)
{
	ZLOX_SINT32 i = 0;
	if(!single_line_out)
	{
		while (c[i])
		{
			zlox_monitor_put(c[i++]);
		}
	}
	else
	{
		ZLOX_UINT8 orig_x = cursor_x;
		ZLOX_UINT8 orig_y = cursor_y;
		while(c[i] && cursor_y == orig_y)
		{
			zlox_monitor_put(c[i++]);
		}
		// 将行尾清空为空格
		if(cursor_y == orig_y)
		{
			ZLOX_CHAR tmp_c = ' ';
			while(cursor_y == orig_y)
			{
				zlox_monitor_put(tmp_c);
			}
		}
		cursor_x = orig_x;
		cursor_y = orig_y;
		zlox_move_cursor();
	}
}

// Outputs an integer as Hex
ZLOX_VOID zlox_monitor_write_hex(ZLOX_UINT32 n)
{
	ZLOX_SINT32 tmp;
	ZLOX_SINT32 i;
	ZLOX_CHAR noZeroes;

	zlox_monitor_write("0x");

	noZeroes = 1;

	for (i = 28; i > 0; i -= 4)
	{
		tmp = (n >> i) & 0xF;
		if (tmp == 0 && noZeroes != 0)
		{
			continue;
		}
	
		if (tmp >= 0xA)
		{
			noZeroes = 0;
			zlox_monitor_put (tmp-0xA+'a' );
		}
		else
		{
			noZeroes = 0;
			zlox_monitor_put( tmp+'0' );
		}
	}
  
	tmp = n & 0xF;
	if (tmp >= 0xA)
	{
		zlox_monitor_put(tmp-0xA+'a');
	}
	else
	{
		zlox_monitor_put(tmp+'0');
	}
}

// Outputs an integer to the monitor.
ZLOX_VOID zlox_monitor_write_dec(ZLOX_UINT32 n)
{
	ZLOX_SINT32 acc;
	ZLOX_CHAR c[32];
	ZLOX_SINT32 i;
	ZLOX_CHAR c2[32];
	ZLOX_SINT32 j;

	if (n == 0)
	{
		zlox_monitor_put('0');
		return;
	}
	
	acc = n;
	i = 0;	
	while (acc > 0)
	{
		c[i] = '0' + acc%10;
		acc /= 10;
		i++;
	}
	c[i] = 0;

	c2[i--] = 0;
	j = 0;
	// reverse the integer string's order
	while(i >= 0)
	{
		c2[i--] = c[j++];
	}
	zlox_monitor_write(c2);
}

ZLOX_VOID zlox_monitor_set_color_space(ZLOX_BOOL flag, ZLOX_UINT32 color, ZLOX_UINT32 backColour,
			ZLOX_SINT32 w_space, ZLOX_SINT32 h_space)
{
	if(flag)
	{
		monitor_color = color;
		monitor_backColour = backColour;
		monitor_w_space = w_space;
		monitor_h_space = h_space;
	}
	else
	{
		monitor_color = ZLOX_MONITOR_COLOR;
		monitor_backColour = ZLOX_MONITOR_BACK_COLOR;
		monitor_w_space = ZLOX_MONITOR_W_SPACE;
		monitor_h_space = ZLOX_MONITOR_H_SPACE;
	}
	return;
}

