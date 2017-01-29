#include <svp/testoutput.h>
#include <svp/mgsim.h>
#include <stdint.h>
#include <stddef.h>

#include "mtconf.h"
#include "MGInputEvents.h"

typedef struct joystickdata //Local joystick info and state
{
  uint8_t naxes;
  uint8_t nbuttons;
  uint8_t nhats;
  uint8_t nballs;
  uint32_t buttons;//Bitset for button state
  int16_t axes[8];//Should cover all reasonable joysticks
  uint8_t hats[8];//Never seen more than one on a device
  int16_t balls[16];//Enough for 8 balls
} joystickdata;

typedef struct joydevctl //simplifies access to control registers
{
  uint8_t enabled;
  uint8_t events;
  uint8_t notifications;
  uint8_t channel;
  uint8_t queuesize;
} joydevctl;

typedef struct joydevinfo //simplifies access to device info
{
  uint32_t axes;
  uint32_t buttons;
  uint32_t hats;
  uint32_t balls;
} joydevinfo;

typedef struct drawcmd //Helps with draw commands on the GPU
{
  uint32_t cmd;
  uint32_t mode;
  uint32_t offset;
  uint32_t scanlen;
  uint32_t size;
  uint32_t pos;
  uint32_t dsize;
} drawcmd;

void delaytime(int i){//Wastes some time
  volatile int t;
  for (t = 0; t < i; t++){}
}

int main(void){
  sys_detect_devs();
  sys_conf_init();

  //Set up helpful pointers
  volatile joydevctl *joydev = (void*)mg_devinfo.base_addrs[mg_joyinput_devids[0]];
  volatile uint8_t *joydevbase = (void*)mg_devinfo.base_addrs[mg_joyinput_devids[0]];
  volatile joydevinfo *joyinfo = (void*)&joydevbase[0x100];
  volatile uint32_t *evdata = (void*)&joydevbase[0x200];
  joydev->enabled = 1;
  joydev->events = 1;
  int i, cmdi = 0; //two counters

  joystickdata js; //Joystick info and state
  MGJoyInputEvent ev;//To store an event
  //Gather information about the joystick
  js.naxes = joyinfo->axes >> 24; //shifting to get the axes count
  js.nbuttons = joyinfo->buttons >> 24;
  js.nhats = joyinfo->hats >> 24;
  js.nballs = joyinfo->balls >> 24;

  //Make sure we can actually draw this joysticks state
  //These limits should not be a problem for most existing joysticks
  if (js.naxes > 8 || js.nbuttons > 32 || js.nhats > 8){
    output_string("Joystick layout breaks visualisation limits\n",1);
    return 0; //Exit the program instead of littering the code with if statements
  }

  //gather data on initial state of axes
  volatile int16_t *axesdata = (void*)&joydevbase[0x400];
  for (i = 0; i < js.naxes; i++)
    js.axes[i] = axesdata[i];

  //gather data on initial state of buttons (always 0 on Linux)
  volatile uint8_t *buttondata = (void*)&joydevbase[0x800];
  for (i = 0; i <= (js.nbuttons - 1) >> 3; i++)
    js.buttons += buttondata[i] << (i * 8);

  //gather data on initial state of hats (always 0 on Linux)
  volatile uint8_t *hatsdata = (void*)&joydevbase[0xc00];
  for (i = 0; i < js.nhats; i++)
    js.hats[i] = hatsdata[i];

  //initialise the graphics device
  mg_gfx_ctl[1] = 640;
  mg_gfx_ctl[2] = 400;
  mg_gfx_ctl[0] = 1;
  mg_gfx_ctl[3] = 5;//Start the command buffer after the palette and button texture
  volatile uint32_t *gfxcmd = (uint32_t*)mg_gfx_fb + 5; //command buffer

  //We put all the colours we use at the start
  volatile uint32_t *palette = (uint32_t*)mg_gfx_fb;
  palette[0] = 0x00000000U;//black
  palette[1] = 0x00ffffffU;//white
  palette[2] = 0x00550000U;//red
  palette[3] = 0x0000aa00U;//green

  //First command clears the screen to black
  gfxcmd[cmdi++] = 0x206;
  gfxcmd[cmdi++] = 0x20;//32 bit colour
  gfxcmd[cmdi++] = 0; //texture starts at 0
  gfxcmd[cmdi++] = 1;
  gfxcmd[cmdi++] = 1 << 16 | 1; //texture size is 1x1
  gfxcmd[cmdi++] = 0; //position is 0,0
  gfxcmd[cmdi++] = 640 << 16 | 400;//Stretch 1 pixel to screen size

  gfxcmd[cmdi++] = 0x102;//set a palette of red/green
  gfxcmd[cmdi++] = 2;//of size 2
  gfxcmd[cmdi++] = 2;//starting at offset 2

  //Location to store button data as a texture
  volatile uint32_t *gfx_buttondata = (uint32_t*)mg_gfx_fb + 4;
  *gfx_buttondata = js.buttons; //store the initial button state

  volatile drawcmd *buttoncmd = (drawcmd*)&gfxcmd[cmdi];
  buttoncmd->cmd = 0x206;
  buttoncmd->mode = 0x10001;//use 1 bit red/green palette
  buttoncmd->offset = 4; //Location of button state
  buttoncmd->scanlen = 32; //32 bits per line
  buttoncmd->size = js.nbuttons << 16 | 1; //only show existing buttons
  buttoncmd->pos = 0; //start at the origin
  buttoncmd->dsize = 640 << 16 | 20;//Stretch the bar to screen width


  //Set up locations of commands for axes/hats
  volatile drawcmd *axescmd = (drawcmd*)&gfxcmd[cmdi+7];
  volatile drawcmd *hatscmd = &axescmd[js.naxes];

  //Axes stretch a single pixel into a white bar
  for (i = 0; i < js.naxes; i++){
    axescmd[i].cmd = 0x206;
    axescmd[i].mode = 0x20;//32-bit colour
    axescmd[i].offset = 1;//Location of white value
    axescmd[i].scanlen = 1;
    axescmd[i].size = 1 << 16 | 1;
    axescmd[i].pos = ((i * 80) + 20) << 16 | 40;
    axescmd[i].dsize = 40 << 16 | (js.axes[i] + 32768) >> 8;
  }

  //Hats show their state with a white block
  //We initialise them centred for now
  for (i = 0; i < js.nhats; i++){
    hatscmd[i].cmd = 0x206;
    hatscmd[i].mode = 0x20;
    hatscmd[i].offset = 1;
    hatscmd[i].scanlen = 1;
    hatscmd[i].size = 1 << 16 | 1;
    hatscmd[i].pos = ((i * 80) + 30) << 16 | 330;
    hatscmd[i].dsize = 20 << 16 | 20;
  }
  mg_gfx_ctl[0] = 1;

  int loop = 1;
  uint32_t *evbuff = (void *)&ev; //To use as a buffer for event data
  while (loop){
    while (joydev->queuesize){
      evbuff[0] = evdata[0];
      evbuff[1] = evdata[1];//Technically this is the only part we need
      evbuff[2] = evdata[2];
      joydev->queuesize = 1;//pop the queue
      if (ev.type == MG_JOYBUTTON && ev.num == 0 && ev.value == 0){
        loop = 0;
        break;//exit the loop if we release button 0
      }
      switch (ev.type){
        case MG_JOYAXISMOTION://Axis value is mapped to [0,255] for display
          js.axes[ev.num] = ev.value;
          axescmd[ev.num].dsize = 40 << 16 | (ev.value + 32768) >> 8;
          break;
        case MG_JOYHATMOTION:
          js.hats[ev.num] = ev.value;
          if (ev.value == 0){//hat in origin
            hatscmd[ev.num].pos = (ev.num * 80) + 30 << 16 | 330;
          } else {//Hat state is decoded to position the square
            int x = 1, y = 1;
            if (ev.value & MG_HAT_UP)
              y = 0;
            else if (ev.value & MG_HAT_DOWN)
              y = 2;
            if (ev.value & MG_HAT_LEFT)
              x = 0;
            else if (ev.value & MG_HAT_RIGHT)
              x = 2;
            hatscmd[ev.num].pos = (ev.num * 80) + 10 + (x * 20) << 16 | 310 + (y * 20);
          }
          break;
        case MG_JOYBUTTON://Button state is updated using masking
          if (ev.value == 0)
            js.buttons &= ~(1L << ev.num);
          else
            js.buttons |= (1L << ev.num);
          *gfx_buttondata = js.buttons; //Copy new data to framebuffer
          break;
        case MG_JOYBALLMOTION://No ball visualisation
        default:
          break;
      }
    }
    delaytime(1000);//waste some time
  }
  joydev->enabled = 0;
  return 0;
}