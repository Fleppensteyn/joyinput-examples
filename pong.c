#include <svp/testoutput.h>
#include <svp/mgsim.h>
#include <stdint.h>
#include <stddef.h>

#include "mtconf.h"
#include "MGInputEvents.h"
#include "gfxlib.h"

void delaytime(int i){//Wastes some time
  volatile int t;
  for (t = 0; t < i; t++){}
}

int main(void){
  sys_detect_devs();
  sys_conf_init();
  if (!gfx_initialize()){
    output_string("Failed to initialise graphical framebuffer",1);
    return 1;
  }
  int x,y;
  gfx_set_resolution(640,480);
  gfx_get_resolution(&x,&y);
  if (x!=640 || y !=480){
    output_string("Failed to set resolution",1);
  }
  //set up background
  gfx_frame *bgf = gfx_alloc_frame(0,0);
  gfx_canvas *bgc = gfx_alloc_canvas(1,1,32);
  gfx_draw_pixel_rgb(bgc,0,0,0,0,0);
  gfx_attach_canvas(bgf, bgc, 0);
  gfx_resize_frame(bgf,640,480);
  gfx_show_frame(bgf);

  //set up paddles and ball
  gfx_canvas *cwhite = gfx_alloc_canvas(1,1,32);
  gfx_draw_pixel_rgb(cwhite,0,0,255,255,255);

  gfx_frame *line = gfx_alloc_frame(315,0),
            *ball = gfx_alloc_frame(316,236),
            *padf[2];
  padf[0] = gfx_alloc_frame(0,200),
  padf[1] = gfx_alloc_frame(630,200);
  gfx_attach_canvas(line, cwhite, 0);
  gfx_attach_canvas(ball, cwhite, 0);
  gfx_attach_canvas(padf[0], cwhite, 0);
  gfx_attach_canvas(padf[1], cwhite, 0);
  gfx_resize_frame(line,10,480);
  gfx_resize_frame(ball,8,8);
  gfx_resize_frame(padf[0],10,80);
  gfx_resize_frame(padf[1],10,80);
  gfx_show_frame(ball);
  gfx_show_frame(padf[0]);
  gfx_show_frame(padf[1]);

  if (mg_joyinput_devcount < 2){
    output_string("Not enough joysticks connected",1);
    return 1;
  }
  volatile uint8_t *joysticks[2];
  volatile int16_t *inputs[2];//Points to axis state
  joysticks[0] = (void*)mg_devinfo.base_addrs[mg_joyinput_devids[0]];
  joysticks[1] = (void*)mg_devinfo.base_addrs[mg_joyinput_devids[1]];
  joysticks[0][0] = 1;//Activate joysticks
  joysticks[1][0] = 1;
  inputs[0] = (void*)&joysticks[0][0x402];//State of axis 1
  inputs[1] = (void*)&joysticks[1][0x402];

  x = 320; y = 240;
  int dx = 1,
      dy = 1,
      pads[2]={200,200},//Pad positions
      dir[2] = {0,0},//Movement direction
      score[2] = {0,0};

  while (1){
    for (int i = 0; i < 2; i++){//Handle user input for both players
      if (*inputs[i]<-4000 && pads[i] > 0){
        pads[i] -= 5;
        dir[i] = -1;
        gfx_move_frame(padf[i], i*630, pads[i]);
      } else if (*inputs[i]>4000 && pads[i] < 400){
        pads[i] += 5;
        dir[i] = 1;
        gfx_move_frame(padf[i], i*630, pads[i]);
      } else {
        dir[i] = 0;//reset direction if the pad is not moving
      }
    }
    x = x+dx;
    y = y+dy;
    if (x > 626){//Check collision right
      if (y < pads[1]-4 || y > pads[1]+84){//Score
        score[0]++;
        x = 320; y= 240;
        dx = -1; dy = (dir[1]>0)?1:-1;
      }
      else {//Collision with pad
        dx = -dx-1;//increase x speed of ball
        if ((dy < 0 &&dir[1]==1) || (dy > 0 && dir[1]==-1))
          dy = -dy+dir[1];//Direction of pad can change y speed
        else
          dy+=((dy>0)?1:-1)+ dir[1];
      }
    }
    else if (x < 14) {//Check collision left
      if (y < pads[0]-4 || y > pads[0]+84){
        score[1]++;
        x = 320; y= 240;
        dx = 1; dy = (dir[0]>0)?1:-1;
      }
      else {
        dx = -dx+1;
        if ((dy < 0 &&dir[0]==1) || (dy > 0 && dir[0]==-1))
          dy= dir[0];
        else
          dy+=((dy>0)?1:-1)+dir[0];
      }
    }
    if (y > 476){dy = -dy;}//Check for y collisions
    else if (y < 4) {dy = -dy;}
    if (joysticks[0][0x800]>0)
      break;//exit on any button press of joystick 1
    gfx_move_frame(ball,x-4,y-4);
    delaytime(1000);//Waste some time
  }

  joysticks[0][0] = 0;
  joysticks[1][0] = 0;
  return 0;
}