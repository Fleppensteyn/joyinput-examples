#include <svp/testoutput.h>
#include <svp/mgsim.h>
#include <stdint.h>
#include <stddef.h>

#include "mtconf.h"
#include "MGInputEvents.h"

volatile uint8_t *joydevbase;

typedef struct sdljoystick
{
  uint8_t naxes;
  uint8_t nbuttons;
  uint8_t nhats;
  uint8_t nballs;
  uint32_t buttons;
  int16_t axes[8];
  uint8_t hats[8];
  int16_t balls[16];
} sdljoystick;

typedef struct joydevctl
{
  uint8_t enabled;
  uint8_t events;
  uint8_t notifications;
  uint8_t channel;
  uint8_t queuesize;
} joydevctl;

typedef struct joydevinfo
{
  uint32_t axes;
  uint32_t buttons;
  uint32_t hats;
  uint32_t balls;
} joydevinfo;

typedef struct drawcmd
{
  uint32_t cmd;
  uint32_t mode;
  uint32_t offset;
  uint32_t scanlen;
  uint32_t size;
  uint32_t pos;
  uint32_t dsize;
}drawcmd;

int main(void){
  sys_detect_devs();
  sys_conf_init();

  volatile joydevctl *joydev = (void*)mg_devinfo.base_addrs[mg_joyinput_devids[0]];
  joydevbase = (void*)mg_devinfo.base_addrs[mg_joyinput_devids[0]];
  volatile joydevinfo *joyinfo = (void*)&joydevbase[0x100];
  volatile uint32_t *evdata = (void*)&joydevbase[0x200];
  joydev->events = 1;
  joydev->enabled = 1;
  int i, cmdi = 0;

  sdljoystick js;
  MGJoyInputEvent ev;
  js.naxes = (joyinfo->axes & 0xff000000) >> 24;
  js.nbuttons = (joyinfo->buttons & 0xff000000) >> 24;
  js.nhats = (joyinfo->hats & 0xff000000) >> 24;
  js.nballs = (joyinfo->balls & 0xff000000) >> 24;

  mg_gfx_ctl[1] = 640;
  mg_gfx_ctl[2] = 400;
  mg_gfx_ctl[0] = 1;
  mg_gfx_ctl[3] = 5;
  volatile uint32_t *gfxcmd = (uint32_t*)mg_gfx_fb + 5;
  for (i = 0; i < 100; i++){
    gfxcmd[i] = 0x0;
  }

  volatile uint32_t *pallette = (uint32_t*)mg_gfx_fb;
  pallette[0] = 0x00000000U;
  pallette[1] = 0x00ffffffU;
  pallette[2] = 0x00550000U;
  pallette[3] = 0x0000aa00U;

  //setup background
  gfxcmd[cmdi++] = 0x206;
  gfxcmd[cmdi++] = 0x20;
  gfxcmd[cmdi++] = 100;
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
  buttoncmd->size = js.nbuttons << 16 | 1;
  buttoncmd->pos = 0;
  buttoncmd->dsize = 640 << 16 | 20;
  // ;
  volatile drawcmd *axescmd =(drawcmd*)&gfxcmd[cmdi+7];
  volatile drawcmd *hatscmd =&axescmd[8];
  for (i = 0; i < 8; i++)
    axescmd[i].cmd = 0x306;

  for (i = 0; i < js.naxes; i++){
    axescmd[i].cmd = 0x206;
    axescmd[i].mode = 0x20;
    axescmd[i].offset = 1;
    axescmd[i].scanlen = 1;
    axescmd[i].size = 1 << 16 | 1;
    axescmd[i].pos = ((i * 80) + 20) << 16 | 40;
    axescmd[i].dsize = 40 << 16;
    js.axes[i] = 0;
  }
  for (i = 0; i < js.nhats; i++){
    hatscmd[i].cmd = 0x206;
    hatscmd[i].mode = 0x20;
    hatscmd[i].offset = 1;
    hatscmd[i].scanlen = 1;
    hatscmd[i].size = 1 << 16 | 1;
    hatscmd[i].pos = ((i * 80) + 30) << 16 | 330;
    hatscmd[i].dsize = 20 << 16 | 20;
    js.hats[i] = 0;
  }

  uint32_t *evbuff = (void *)&ev;
  while (1){
    if (joydev->queuesize > 0){
      evbuff[0] = evdata[0];
      evbuff[1] = evdata[1];
      evbuff[2] = evdata[2];
      if (ev.type == MG_JOYBUTTON && ev.num == 0 && ev.value == 0)
        break;
      switch (ev.type){
        case MG_JOYAXISMOTION:
          js.axes[ev.num] = ev.value;
          uint32_t temp = (ev.value + 32768) >> 8;
          axescmd[ev.num].dsize = 40 << 16 | temp;
          break;
        case MG_JOYHATMOTION:
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
            hatscmd[ev.num].pos = (ev.num * 80) + 10 + (x * 20) << 16 | 310 + (y * 20);
          }
          break;
        case MG_JOYBUTTON:
          if (ev.value == 0)
            js.buttons &= ~(1L << ev.num);
          else
            js.buttons |= (1L << ev.num);
          *gfx_buttondata = js.buttons;
          break;
        case MG_JOYBALLMOTION://No ball visualisation
        default:
          break;
      }
      joydev->queuesize = 1;
    }
  }
  joydev->enabled = 0;
  return 0;
}