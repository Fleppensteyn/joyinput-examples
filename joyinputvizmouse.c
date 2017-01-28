#include <svp/testoutput.h>
#include <svp/mgsim.h>
#include <stdint.h>
#include <stddef.h>

#include "mtconf.h"
#include "MGInputEvents.h"

volatile uint8_t *joydevbase;

typedef struct joydevctl
{
  uint8_t enabled;
  uint8_t events;
  uint8_t notifications;
  uint8_t channel;
  uint8_t queuesize;
} joydevctl;

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

int16_t nabs(int16_t i){//absolute value
  return (i < 0)?-i:i;
}

int main(void){
  sys_detect_devs();
  sys_conf_init();

  volatile joydevctl *joydev = (void*)mg_devinfo.base_addrs[mg_joyinput_devids[0]];
  joydevbase = (void*)mg_devinfo.base_addrs[mg_joyinput_devids[0]];
  volatile uint32_t *evdata = (void*)&joydevbase[0x200];
  joydev->events = 1;
  joydev->enabled = 1;
  int cmdi = 0;
  MGMouseInputEvent ev;
  volatile int16_t *axesdata = (void*)&joydevbase[0x400];
  volatile int16_t *ballsdata = (void*)&joydevbase[0x1000];

  mg_gfx_ctl[1] = 640;
  mg_gfx_ctl[2] = 400;
  mg_gfx_ctl[0] = 1;
  mg_gfx_ctl[3] = 3;
  volatile uint32_t *gfxcmd = (uint32_t*)mg_gfx_fb + 3;

  volatile uint32_t *pallette = (uint32_t*)mg_gfx_fb;
  pallette[0] = 0x00000000U;
  pallette[1] = 0x00ffffffU;
  pallette[2] = 0x0000aa00U;

  gfxcmd[cmdi++] = 0x206;
  gfxcmd[cmdi++] = 0x20;
  gfxcmd[cmdi++] = 0;
  gfxcmd[cmdi++] = 1;
  gfxcmd[cmdi++] = 1 << 16 | 1;
  gfxcmd[cmdi++] = 0;
  gfxcmd[cmdi++] = 640 << 16 | 400;

  //Point placed at the mouse pointer position
  volatile drawcmd *mousepoint = (void*)&gfxcmd[cmdi];
  mousepoint->cmd = 0x206;
  mousepoint->mode = 0x20;
  mousepoint->offset = 1;
  mousepoint->scanlen = 1;
  mousepoint->size = 1 << 16 | 1;
  mousepoint->pos = 0;
  mousepoint->dsize = 3 << 16 | 3;
  //Box indicating relative movement
  volatile drawcmd *trail = (void*)&gfxcmd[cmdi+7];
  trail->cmd = 0x206;
  trail->mode = 0x20;
  trail->offset = 2;
  trail->scanlen = 1;
  trail->size = 1 << 16 | 1;
  trail->pos = 0;
  trail->dsize = 0 << 16 | 0;

  uint32_t *evbuff = (void*)&ev;
  while (1){
    if (joydev->queuesize > 0){
      evbuff[1] = evdata[1];//The only part we need
      joydev->queuesize = 1;
      if (ev.type == MG_MOUSEMOTION){
        int posx = axesdata[0], posy = axesdata[1];
        mousepoint->pos = posx << 16 | posy;
        int offx = ballsdata[0], offy = ballsdata[1];
        if (offx!=0 || offy!=0){//update trail box
          trail->dsize = (nabs(offx)+1) << 16 | nabs(offy)+1;
          trail->pos = (posx+offx) << 16 | posy+offy;
        }
      }
      if (ev.type == MG_MOUSEBUTTON && ev.num == MG_BUTTON_RIGHT && ev.state == 0)
        break;//exit on right button release
    }
  }
  joydev->enabled = 0;
  return 0;
}