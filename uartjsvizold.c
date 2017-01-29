#include <svp/testoutput.h>
#include <svp/mgsim.h>
#include <stdint.h>
#include <stddef.h>

#include "mtconf.h"

volatile uint8_t *uartctl; //points to the uart base

typedef struct jsev //Linux joystick API event
{
    uint32_t time;
    int16_t value;
    uint8_t type;
    uint8_t num;
} jsev;

typedef struct joystick //joystick info and state
{
  uint8_t nbuttons;
  uint8_t naxes;
  uint32_t buttons;
  int16_t axes[8];
} joystick;

typedef struct drawcmd
{
  uint32_t cmd;
  uint32_t mode;
  uint32_t offset;
  uint32_t scanlen;
  uint32_t size;
  uint32_t pos;
  uint32_t dsize;
} drawcmd;

//Check for and copy an event if available
//Data is copied into the provided event
//returns 0 when there is none, 1 otherwise
int get_event(jsev *event){
  if ((uartctl[5] & 1) == 0)
    return 0;
  uint8_t *buff = (void*)event;
  int i;
  for (i = 0; i < 8; i++){
    buff[i] = uartctl[0];
  }
  return 1;
}

int main(void){
  sys_detect_devs();
  sys_conf_init();

  volatile uint64_t *mgctl = (void*)0x268;
  uartctl = (void*)mg_devinfo.base_addrs[mg_uart_devid];
  uartctl[10] = 1;

  jsev ev;
  joystick js;

  mg_gfx_ctl[1] = 640;
  mg_gfx_ctl[2] = 400;
  mg_gfx_ctl[0] = 1;
  mg_gfx_ctl[3] = 5;
  volatile uint32_t *gfxcmd = (uint32_t*)mg_gfx_fb + 5;
  int i, cmdi = 0;

  volatile uint32_t *palette = (uint32_t*)mg_gfx_fb;
  palette[0] = 0x00000000U;
  palette[1] = 0x00ffffffU;
  palette[2] = 0x00550000U;
  palette[3] = 0x0000aa00U;

  //set up background
  gfxcmd[cmdi++] = 0x206;
  gfxcmd[cmdi++] = 0x20;
  gfxcmd[cmdi++] = 0;
  gfxcmd[cmdi++] = 1;
  gfxcmd[cmdi++] = 1 << 16 | 1;
  gfxcmd[cmdi++] = 0;
  gfxcmd[cmdi++] = 640 << 16 | 400;

  gfxcmd[cmdi++] = 0x102;
  gfxcmd[cmdi++] = 2;
  gfxcmd[cmdi++] = 2;

  volatile uint32_t *gfx_buttondata = (uint32_t*)mg_gfx_fb + 4;
  volatile drawcmd *buttoncmd = (drawcmd*)&gfxcmd[cmdi];
  buttoncmd->cmd = 0x206;
  buttoncmd->mode = 0x10001;
  buttoncmd->offset = 4;
  buttoncmd->scanlen = 32;
  buttoncmd->size = 1 << 16 | 1;
  buttoncmd->pos = 0;
  buttoncmd->dsize = 640 << 16 | 20;

  volatile drawcmd *axescmd =(drawcmd*)&gfxcmd[cmdi+7];
  while (1){
    if (get_event(&ev)){
      output_string("Got event: ",1);
      if (ev.type == 1 && ev.num == 0 && ev.value == 0)
        break;//exit when button 0 is released
      if (ev.type > 2){ //Catches the initial events
        if (ev.type == 0x81){//Button event
          js.nbuttons = ev.num + 1;
          buttoncmd->size = js.nbuttons << 16 | 1;
        } else { //axis event, this creates a new draw command
          js.naxes = ev.num + 1;
          axescmd[ev.num].cmd = 0x206;
          axescmd[ev.num].mode = 0x20;
          axescmd[ev.num].offset = 1;
          axescmd[ev.num].scanlen = 1;
          axescmd[ev.num].size = 1 << 16 | 1;
          axescmd[ev.num].pos = ((ev.num * 80) + 20) << 16 | 40;
          axescmd[ev.num].dsize = 40 << 16;
        }
        ev.type -= 0x80; //convert it to a normal event for handling
      }
      if (ev.type == 1){
        output_string("Button ",1);
        output_uint(ev.num,1);
        if (ev.value == 0){
          output_string(" released",1);
          js.buttons &= ~(1L << ev.num);
        }
        else{
          output_string(" pressed",1);
          js.buttons |= (1L << ev.num);
        }
        *gfx_buttondata = js.buttons;
      } else {
        output_string("Axis ",1);
        output_uint(ev.num,1);
        output_string(" moved to ",1);
        output_int(ev.value,1);
        js.axes[ev.num] = ev.value;
        uint32_t temp = (ev.value + 32767) >> 8;
        axescmd[ev.num].dsize = 40 << 16 | temp;
      }
      output_char('\n', 1);
    }
  }
  uartctl[1] = 0;
  uartctl[10] = 0;
  return 0;
}