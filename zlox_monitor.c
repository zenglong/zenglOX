/*zlox_monitor.c Define some functions for writing to the monitor*/

#include "zlox_monitor.h"

// The VGA framebuffer starts at 0xB8000.
ZLOX_UINT16 * video_memory = (ZLOX_UINT16 *)0xB8000;
// Stores the cursor position.
ZLOX_UINT8 cursor_x = 0;
ZLOX_UINT8 cursor_y = 0;

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
        for (i = 0*80; i < 24*80; i++)
        {
            video_memory[i] = video_memory[i+80];
        }

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

// Writes a single character out to the screen.
ZLOX_VOID zlox_monitor_put(ZLOX_CHAR c)
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
    if (c == 0x08 && cursor_x)
    {
        cursor_x--;
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

    // Scroll the screen if needed.
    zlox_scroll();
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
ZLOX_VOID zlox_monitor_write(ZLOX_CHAR * c)
{
    ZLOX_SINT32 i = 0;
    while (c[i])
    {
        zlox_monitor_put(c[i++]);
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

