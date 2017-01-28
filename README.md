# joyinput-examples
Example programs and configurations to test the mgsim-joyinput interface.

##Programs

### ``joyinput.c``
Reads events from all connected joyinput devices and describes them with text.
Recommended config: ``config_dualjsandmouse.ini``.
###``joyinputviz.c``
Visualises the joystick data from the first joyinput device, use when the first joyinput device is a joystick.
Recommended config: ``config_sdljoy0.ini``.
###``joyinputvizmouse.c``
Shows a point that chases the mouse cursor and shows the relative movement since the last update, use when the first joyinput device is a mouse.
Recommended config: ``config_sdlmouse.ini``.
###``multijoytest.c``
Moves dots around the screen for up to four connected joyinput devices based on the values from the first two absolute axes. If a mouse is connected it chases the cursor.
Recommended config: ``config_dualjs.ini`` or ``config_dualjsandmouse.ini``.
###``pong.c``
Crude pong implementation that uses the graphics drivers and joyinput to show game potential, could be greatly improved.
Recommended config: ``config_dualjs.ini``.
###``uartjsvizold.c``
Uses the linux joystick API through the uart and visualises it.
Recommended config: ``config_uartjslinux.ini``.
###``uartjsviznew.c``
Uses the joyinput events through the UART to do visualise the joystick.
Recommended config: ``config_uartjsnew.ini``.
###``joyinputviznoint.c``
Visualises the joystick data from the first joyinput device, but does not use interrupt functionality and waits a while instead. This can, for instance, be used to visualise replay functionality.
Recommended config: ``config_recordjs0.ini`` followed by ``config_replayjs0.ini``.

##Configs
###``config_sdljoy0.ini``
Has one joyinput device that connects to SDL joystick 0.
###``config_sdlmouse.ini``
Has one joyinput device that connects to the mouse.
###``config_dualjs.ini``
Has two joyinput devices that connect to SDL joysticks 0 and 1.
###``config_dualjsandmouse.ini``
Has three joyinput devices that connect to SDL joysticks 0 and 1, and the mouse.
###``config_recordjs0.ini``
Has a single joyinput device that records a replay and connects to SDL joystick 0.
###``config_replayjs0.ini``
Replays the log saved by ``config_recordjs0.ini``.
###``config_uartjslinux.ini``
Uses the Linux joystick API to connect the UART to ``/dev/input/js0``.
###``config_uartjsnew.ini``
Uses SDL to connect the UART to SDL joystick 0.
