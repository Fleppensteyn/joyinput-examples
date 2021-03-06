#include <svp/testoutput.h>
#include <svp/mgsim.h>
#include <stdint.h>
#include <stddef.h>

#include "mtconf.h"
#include "MGInputEvents.h"

volatile uint8_t *uartctl; //points to the uart base

typedef struct joystick //joystick info and state
{
  uint8_t nbuttons;
  uint8_t naxes;
  uint8_t nhats;
  uint32_t buttons;
  int16_t axes[8];
  uint8_t hats[8];
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
int get_event(MGJoyInputEvent *event){
  if ((uartctl[5] & 1) == 0)
    return 0;
  uint8_t *buff = (void*)event;
  int i;
  for (i = 0; i < 10; i++){
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

  MGJoyInputEvent ev;
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
  volatile drawcmd *hatscmd =&axescmd[8];
  for (i = 0; i < 8; i++)
    axescmd[i].cmd = 0x306;

  while (1){
    if (get_event(&ev)){
      output_string("Got event: ",1);
      if (ev.type == MG_JOYBUTTON && ev.num == 0 && ev.value == 0)
        break; //exit on button 0 release
      switch (ev.type){
        case MG_JOYAXISMOTION:
        //Because we can't query the device for the amount of axes
        //and we don't receive an initial state we have to dynamically
        //expand the amount of axes we draw based on the events we receive
          if (js.naxes <= ev.num){
            for (; js.naxes <= ev.num; js.naxes++){
              axescmd[js.naxes].cmd = 0x206;
              axescmd[js.naxes].mode = 0x20;
              axescmd[js.naxes].offset = 1;
              axescmd[js.naxes].scanlen = 1;
              axescmd[js.naxes].size = 1 << 16 | 1;
              axescmd[js.naxes].pos = ((js.naxes * 80) + 20) << 16 | 40;
              axescmd[js.naxes].dsize = 40 << 16;
              js.axes[js.naxes] = 0;
            }
          }
          output_string("Axis ",1);
          output_uint(ev.num,1);
          output_string(" moved to ",1);
          output_int(ev.value,1);
          js.axes[ev.num] = ev.value;
          uint32_t temp = (ev.value + 32768) >> 8;
          axescmd[ev.num].dsize = 40 << 16 | temp;
          break;
        case MG_JOYHATMOTION:
          //Just like the axes this creates hat draw commands
          if (js.nhats <= ev.num){
              for (; js.nhats <= ev.num; js.nhats++){
              hatscmd[js.nhats].cmd = 0x206;
              hatscmd[js.nhats].mode = 0x20;
              hatscmd[js.nhats].offset = 1;
              hatscmd[js.nhats].scanlen = 1;
              hatscmd[js.nhats].size = 1 << 16 | 1;
              hatscmd[js.nhats].pos = ((js.nhats * 80) + 30) << 16 | 330;
              hatscmd[js.nhats].dsize = 20 << 16 | 20;
              js.hats[js.nhats] = 0;
            }
          }
          output_string("Hat ",1);
          output_uint(ev.num,1);
          output_string(" moved to state ",1);
          output_int(ev.value,1);
          js.hats[ev.num] = ev.value;
          if (ev.value == 0){
            hatscmd[ev.num].pos = (ev.num * 80) + 30 << 16 | 330;
          } else {
            int x = 1, y = 1;
            if ((ev.value & MG_HAT_UP) != 0)
              y = 0;
            else if ((ev.value & MG_HAT_DOWN) != 0)
              y = 2;
            if ((ev.value & MG_HAT_LEFT) != 0)
              x = 0;
            else if ((ev.value & MG_HAT_RIGHT) != 0)
              x = 2;
            output_int(x,1);
            output_char(' ',1);
            output_int(y,1);
            hatscmd[ev.num].pos = (ev.num * 80) + 10 + (x * 20) << 16 | 310 + (y * 20);
          }
          break;
        case MG_JOYBUTTON:
          if (js.nbuttons <= ev.num){ //expand buttons
            js.nbuttons = ev.num+1;
            buttoncmd->size = js.nbuttons << 16 | 1;
          }
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
          break;
        case MG_JOYBALLMOTION:
          output_string("Ball ",1);
          output_uint(ev.num,1);
          output_string(" moved ",1);
          output_int(ev.value,1);
          output_char(' ',1);
          output_int(ev.value2,1);
          break;
        default:
          break;
      }
      output_char('\n', 1);
    }
  }
  uartctl[1] = 0;
  uartctl[10] = 0;
  return 0;
}