#include <svp/testoutput.h>
#include <svp/mgsim.h>
#include <stdint.h>
#include <stddef.h>

#include "mtconf.h"
#include "MGInputEvents.h"

volatile uint8_t *joydevbase[4];

typedef struct playerpos
{
  int x;
  int y;
  int scale;
} playerpos;

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


int main(void){
  sys_detect_devs();
  sys_conf_init();

  if (mg_joyinput_devcount < 2){
    output_string("Expected multiple JoyInput devices\n",1);
    return 1;
  }
  playerpos pos[4];
  int i, cmdi = 0;
  for (i = 0; i < mg_joyinput_devcount; i++){
    joydevbase[i] = (void*)mg_devinfo.base_addrs[mg_joyinput_devids[i]];
    joydevbase[i][0] = 1;
    pos[i].x = 320;
    pos[i].y = 240;
    pos[i].scale = (joydevbase[i][0] == 1)?12:6;
  }

  mg_gfx_ctl[1] = 640;
  mg_gfx_ctl[2] = 480;
  mg_gfx_ctl[0] = 1;
  mg_gfx_ctl[3] = 5;
  volatile uint32_t *gfxcmd = (uint32_t*)mg_gfx_fb + 5;
  for (i = 0; i < 100; i++){
    gfxcmd[i] = 0x0;
  }

  volatile uint32_t *pallette = (uint32_t*)mg_gfx_fb;
  pallette[0] = 0x00000000U;
  pallette[1] = 0x00ffffffU;
  pallette[2] = 0x00ff0000U;
  pallette[3] = 0x0000ff00U;
  pallette[4] = 0x000000ffU;

  //setup background
  gfxcmd[cmdi++] = 0x206;
  gfxcmd[cmdi++] = 0x20;
  gfxcmd[cmdi++] = 100;
  gfxcmd[cmdi++] = 1;
  gfxcmd[cmdi++] = 1 << 16 | 1;
  gfxcmd[cmdi++] = 0;
  gfxcmd[cmdi++] = 640 << 16 | 480;

  volatile drawcmd *points[4];
  for (i = 0; i < mg_joyinput_devcount; i++){
    points[i] = &gfxcmd[cmdi+(7*i)];
    points[i]->cmd = 0x206;
    points[i]->mode = 0x20;
    points[i]->offset = 1+i;
    points[i]->scanlen = 1;
    points[i]->size = 1 << 16 | 1;
    points[i]->pos = pos[i].x << 16 | pos[i].y;
    points[i]->dsize = 3 << 16 | 3;
  }
  volatile int delay=0;
  while (1){
    for (i = 0; i < mg_joyinput_devcount; i++){
      int16_t dx = *(int16_t*)(void*)&joydevbase[i][0x400],
              dy = *(int16_t*)(void*)&joydevbase[i][0x402];
      pos[i].x += dx >> pos[i].scale;
      pos[i].y += dy >> pos[i].scale;
      if (pos[i].x < 0) pos[i].x +=640;
      else if (pos[i].x > 640) pos[i].x -=640;
      if (pos[i].y < 0) pos[i].y +=480;
      else if (pos[i].y > 480) pos[i].y -=480;
      points[i]->pos = pos[i].x << 16 | pos[i].y;
    }
    if (joydevbase[0][0x800] > 0)
      break;
    for (i=0;i<1000;i++){
      delay = i;
    }
  }
  for (i = 0; i < mg_joyinput_devcount; i++){
    joydevbase[i][0] = 0;
  }
  return 0;
}