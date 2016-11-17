#include <svp/testoutput.h>
#include <svp/mgsim.h>
#include <stdint.h>
#include <stddef.h>

#include "mtconf.h"
#include "MGInputEvents.h"

// Helpful for controlling the device
typedef struct joydevctl
{
  uint8_t enabled;
  uint8_t events;
  uint8_t notifications;
  uint8_t channel;
  uint8_t queue;
} joydevctl;

//More descriptive access to the device info part
typedef struct joydevinfo
{
  uint32_t axes;
  uint32_t buttons;
  uint32_t hats;
  uint32_t balls;
} joydevinfo;

typedef struct joydev
{
  volatile uint8_t* base; //base of the device adress space
  volatile joydevctl* ctl; //control structure simplifies access
  volatile joydevinfo* info; //struct to help access to device info
  volatile uint32_t* ev; //Points to memory space for events at the correct access width
  volatile int16_t* axes; //Pointer for direct access to axe state
  volatile uint8_t* buttons;//Pointer for direct access to button state
  volatile uint8_t* hats;//Pointer for direct access to hat state
  volatile int16_t* balls;//Pointer for direct access to ball state
  uint8_t type; //cache the device type locally to avoid accessing the interface.
} joydev;

int main(void){
  sys_detect_devs();
  sys_conf_init();
  //create an empty screen to allow SDL to receive events.
  mg_gfx_ctl[1] = 640;
  mg_gfx_ctl[2] = 400;
  mg_gfx_ctl[0] = 1;


  output_string("Example for using JoyInput.\n",1);
  if (mg_joyinput_devcount < 1){
    output_string("Please add a JoyInput device to the configuration for this demo.\n",1);
    return 0;
  }
  output_string("Detected ",1);
  output_uint(mg_joyinput_devcount,1);
  output_string(" device(s).\n",1);

  joydev dev[4];
  int i;
  for (i = 0; i < mg_joyinput_devcount; i++){
    dev[i].base = (void*)mg_devinfo.base_addrs[mg_joyinput_devids[i]];
    dev[i].ctl = (void*)&dev[i].base[0];
    dev[i].info = (void*)&dev[i].base[0x100];
    dev[i].ev = (void*)&dev[i].base[0x200];
    dev[i].axes = (void*)&dev[i].base[0x400];
    dev[i].buttons = (void*)&dev[i].base[0x800];
    dev[i].hats = (void*)&dev[i].base[0xc00];
    dev[i].balls = (void*)&dev[i].base[0x1000];
    dev[i].ctl->enabled = 1;
    dev[i].ctl->events = 1;
    output_string("Device ",1);
    output_int(i+1,1);
    dev[i].type = dev[i].ctl->enabled;
    switch (dev[i].type){
      case 1://Joystick
        output_string(" is a joystick with:\n",1);
        output_uint((dev[i].info->axes >> 24),1);
        output_string(" axes, ",1);
        output_uint((dev[i].info->buttons >> 24),1);
        output_string(" buttons, ",1);
        output_uint((dev[i].info->hats >> 24),1);
        output_string(" hats, ",1);
        output_uint((dev[i].info->balls >> 24),1);
        output_string(" balls\n",1);
        break;
      case 2: output_string(" is the mouse.\n",1); break;
      case 3: output_string(" is a touch device.\n",1); break;
      case 0: output_string(" is not functioning?.\n",1); break;
      default: output_string(" is not a known type.\n",1); break;
    }
  }
  MGInputEvent ev;
  uint32_t *evp = (uint32_t*)&ev; //Used to copy bytes from the interface to the local event.
  int evsize[4] = {0,3,4,5}; //size of events for the different device types to avoid copying more bytes than needed.
  int loop = 1;
  output_string("Grabbing events from all connected devices until button 0 is pressed on any device or you triple click somewhere.\n",1);
  output_string("Format: timestamp|device number|event type|num|value(s)\n",1);
  while (loop){
    for (i = 0; i < mg_joyinput_devcount; i++){
      while (dev[i].ctl->queue){//Check the queue for events
        for (int j = 0; j < evsize[dev[i].type]; j++)
            evp[j] = dev[i].ev[j];
        output_uint(ev.common.timestamp,1);
        output_char('|',1);
        output_int(i+1,1);
        output_char('|',1);
        switch (ev.common.type){
          case MG_JOYAXISMOTION:
            output_string("Joystick axis|",1);
            output_uint(ev.joy.num,1);
            output_char('|',1);
            output_int(ev.joy.value,1);
            break;
          case MG_JOYBUTTON:
            output_string("Joystick button|",1);
            output_uint(ev.joy.num,1);
            output_char('|',1);
            output_string(ev.joy.value?"Pressed":"Released",1);
            if (ev.joy.num == 0 && ev.joy.value == 0) loop = 0;
            break;
          case MG_JOYHATMOTION:
            output_string("Joystick hat|",1);
            output_uint(ev.joy.num,1);
            output_char('|',1);
            output_int(ev.joy.value,1);
            output_string(" = ",1);
            if (ev.joy.value&MG_HAT_LEFT) output_char('<',1);
            if (ev.joy.value&MG_HAT_UP) output_char('^',1);
            if (ev.joy.value&MG_HAT_DOWN) output_char('v',1);
            if (ev.joy.value&MG_HAT_RIGHT) output_char('>',1);
            break;
          case MG_JOYBALLMOTION:
            output_string("Joystick ball|",1);
            output_uint(ev.joy.num,1);
            output_char('|',1);
            output_int(ev.joy.value,1);
            output_char(',',1);
            output_int(ev.joy.value2,1);
            break;
          case MG_MOUSEMOTION:
            output_string("Mouse motion|0|Moved [",1);
            output_int(ev.mouse.xrel,1);
            output_char(',',1);
            output_int(ev.mouse.yrel,1);
            output_string("] to [",1);
            output_int(ev.mouse.xpos,1);
            output_char(',',1);
            output_int(ev.mouse.ypos,1);
            output_string("] button state [",1);
            output_uint(ev.mouse.state,1);
            output_char(']',1);
            break;
          case MG_MOUSEBUTTON:
            output_string("Mouse button|",1);
            output_uint(ev.mouse.num,1);
            output_string("|",1);
            output_string(ev.mouse.state?"Pressed ":"Released ", 1);
            output_uint(ev.mouse.clicks,1);
            output_string(" time(s) at [",1);
            output_int(ev.mouse.xpos,1);
            output_char(',',1);
            output_int(ev.mouse.ypos,1);
            output_char(']',1);
            if (ev.mouse.state == 0 && ev.mouse.clicks == 3) loop = 0;
            break;
          case MG_MOUSEWHEEL:
            output_string("Mouse wheel|0|Moved",1);
            output_int(ev.mouse.xrel,1);
            output_char(',',1);
            output_int(ev.mouse.yrel,1);
            output_char(']',1);
          case MG_TOUCHMOTION:
          case MG_TOUCHUP:
          case MG_TOUCHDOWN:
            output_string("Touch ",1);
            output_string((ev.common.type == MG_TOUCHMOTION ? "motion|":
                          (ev.common.type == MG_TOUCHUP)?"up|":"down|"),1);
            output_uint(ev.touch.device,1);
            output_string("|Finger ",1);
            output_uint(ev.touch.num,1);
            output_string(" at [",1);
            output_int(ev.touch.xpos,1);
            output_char(',',1);
            output_int(ev.touch.ypos,1);
            output_string("] moved [",1);
            output_int(ev.touch.xrel,1);
            output_char(',',1);
            output_int(ev.touch.yrel,1);
            output_string("] Pressure: ",1);
            output_int(ev.touch.pressure,1);
            break;
          default:
            output_string("Unknown event type",1);
            break;
        }
        output_char('\n',1);
        dev[i].ctl->queue = 1; //pop the queue for this device.
      }
    }
  }
  output_string("Event loop ended, reading device state and exiting.\n",1);
  for (i = 0; i < mg_joyinput_devcount; i++){
    output_string("Device state ",1);
    output_int(i+1,1);
    if (dev[i].type == 3){
      output_string(": No device state is kept for touch devices",1);
    } else if (dev[i].type == 2){
      output_string(": Last mouse position: [",1);
      output_int(dev[i].axes[0],1);
      output_char(',',1);
      output_int(dev[i].axes[1],1);
      output_string("] Last movement: [",1);
      output_int(dev[i].balls[0],1);
      output_char(',',1);
      output_int(dev[i].balls[1],1);
      output_string("] Final button state: ",1);
      output_uint(dev[i].buttons[0],1);
    } else if (dev[i].type == 1){
      output_string(": ",1);
      uint8_t c = dev[i].info->axes >> 24; //temporarily store axis count
      if (c){
        output_string("Axes: [",1);
        output_int(dev[i].axes[0],1);
        for (uint8_t j = 1; j < c; j++){
          output_char(',',1);
          output_int(dev[i].axes[j],1);
        }
        output_string("] ",1);
      }
      c = dev[i].info->buttons >> 24;
      if (c){
        output_string("Buttons: ",1);
        output_uint(dev[i].buttons[0],1);
        for (uint8_t j = 0; j < ((c-1) >> 3); j++){
          output_char(',',1);
          output_uint(dev[i].buttons[j+1],1);
        }
        output_string(" ",1);
      }
      c = dev[i].info->hats >> 24;
      if (c){
        output_string("Hats: [",1);
        output_uint(dev[i].hats[0],1);
        for (uint8_t j = 1; j < c; j++){
          output_char(',',1);
          output_uint(dev[i].hats[j],1);
        }
        output_string("] ",1);
      }
      c = dev[i].info->balls >> 24;
      if (c){
        output_string("Balls: [(",1);
        output_int(dev[i].balls[0],1);
        output_char(',',1);
        output_int(dev[i].balls[1],1);
        output_char(')',1);
        for (uint8_t j = 1; j < c; j++){
          output_char('(',1);
          output_int(dev[i].balls[j*2],1);
          output_char(',',1);
          output_int(dev[i].balls[j*2+1],1);
          output_char(')',1);
        }
        output_string("]",1);
      }
    }
    dev[i].ctl->enabled = 0; //disable device before quitting
    output_char('\n',1);
  }
  return 0;
}