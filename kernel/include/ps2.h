#pragma once

#define PS2_DATA_REGISTER 0x60
#define PS2_STATUS_REGISTER 0x64
#define PS2_COMMAND_REGISTER 0x64

#define PS2_CMD_GET_CONTROLLER_CONFIG 0x20
#define PS2_CMD_SET_CONTROLLER_CONFIG 0x60

void init_ps2(void);
void init_keyboard(void);
void init_mouse(void);
