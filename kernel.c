#include "kernel.h"
#include "utils.h"
#include "char.h"

#include "stdio.h"
#include "math.h"
#include "string.h"
#include "linux/time.h"

uint32 vga_index;
static uint32 next_line_index = 1;
uint8 g_fore_color = WHITE, g_back_color = BLUE;
int digit_ascii_codes[10] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
char str[128] = {0};
int strc = 0;

uint8 inb(uint16 port)
{
  uint8 ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "d"(port));
  return ret;
}

void outb(uint16 port, uint8 data)
{
  asm volatile("outb %0, %1" : "=a"(data) : "d"(port));
}

/*
this is same as we did in our assembly code for vga_print_char

vga_print_char:
  mov di, word[VGA_INDEX]
  mov al, byte[VGA_CHAR]

  mov ah, byte[VGA_BACK_COLOR]
  sal ah, 4
  or ah, byte[VGA_FORE_COLOR]

  mov [es:di], ax

  ret

*/
uint16 vga_entry(unsigned char ch, uint8 fore_color, uint8 back_color) 
{
  uint16 ax = 0;
  uint8 ah = 0, al = 0;

  ah = back_color;
  ah <<= 4;
  ah |= fore_color;
  ax = ah;
  ax <<= 8;
  al = ch;
  ax |= al;

  return ax;
}

void clear_vga_buffer(uint16 **buffer, uint8 fore_color, uint8 back_color)
{
  uint32 i;
  for(i = 0; i < BUFSIZE; i++){
    (*buffer)[i] = vga_entry(' ', fore_color, back_color);
  }
  next_line_index = 1;
  vga_index = 0;

}

void init_vga(uint8 fore_color, uint8 back_color)
{
  vga_buffer = (uint16*)VGA_ADDRESS;
  clear_vga_buffer(&vga_buffer, fore_color, back_color);
  g_fore_color = fore_color;
  g_back_color = back_color;
}

void print_new_line()
{
	if(next_line_index > (BUFSIZE / 80) - 3){ // -3?
    		next_line_index = 0;
    		clear_vga_buffer(&vga_buffer, WHITE, BLUE);
  	}
  	vga_index = 80 * next_line_index;
  	next_line_index++;
}

void print_char(char ch)
{
	vga_buffer[vga_index] = vga_entry(ch, g_fore_color, g_back_color);
	vga_index++;
}

void print_string(char *str)
{
  	uint32 index = 0;
  	while(str[index]){
    		print_char(str[index]);
    		index++;
  	}
}

void print_int(int num)
{
  	char str_num[digit_count(num)+1];
  	itoa(num, str_num);
  	print_string(str_num);
}

char get_input_keycode()
{
  char ch = 0;
  while((ch = inb(KEYBOARD_PORT)) != 0){
    if(ch > 0)
      return ch;
  }
  return ch;
}

/*
keep the cpu busy for doing nothing(nop)
so that io port will not be processed by cpu
here timer can also be used, but lets do this in looping counter
*/
void wait_for_io(uint32 timer_count)
{
  while(1){
    asm volatile("nop");
    timer_count--;
    if(timer_count <= 0)
      break;
    }
}

void sleep(uint32 timer_count)
{
  wait_for_io(timer_count);
}


void loop()
{

  char ch = 0;
  char keycode = 0;

  do {

 	keycode = get_input_keycode();

    	if(keycode == KEY_ENTER){

		if(strlen(str) > 0) {
      			print_new_line();
      			print_string(str);
		}

		if(strcmp(str, "TIME") == 0) {
			print_new_line();
			print_string("no time available");
		}

		print_new_line();
		print_string("> ");

      		strc = 0;
      		for(int i = 0; i < 128; i++)
			str[i] = 0;

    	} 

	else if(keycode == KEY_BACKSPACE) {

		if(vga_index > 80*(next_line_index-1) + 2 ) {
			vga_index--;
			strc--;
			vga_buffer[vga_index] = vga_entry(' ', WHITE, BLUE);
		}

	}

	else if(keycode == KEY_SPACE) {
	
		str[strc] = ' ';
      		strc++;
      		print_char(' ');	

	}

	else {

      		ch = get_ascii_char(keycode);

		if( ch == KEY_SPACE || (ch >= 65 && ch <= 90) || (ch >= 48 && ch <= 57) ) {
      			str[strc] = ch;
      			strc++;
      			print_char(ch);
		}

	}
    
    	sleep(0x02FFFFFF);

  } while(1);

}

void kernel_entry()
{

  init_vga(WHITE, BLUE);
  loop();

	/*vga_buffer = (uint16 *)VGA_ADDRESS;
  	clear_vga_buffer(&vga_buffer, WHITE, BLACK);
	vga_buffer[0] = vga_entry('X', WHITE, BLACK);
  	vga_index++;
	setCursorPos();
	*/

}

