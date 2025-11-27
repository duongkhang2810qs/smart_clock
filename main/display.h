#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void vDisplayHardwareInit( void );   // init SPI + MAX7219 matrix
void vDisplayTask( void *pvParameters );  // task hiển thị HHMM trên 8x32

#ifdef __cplusplus
}
#endif
