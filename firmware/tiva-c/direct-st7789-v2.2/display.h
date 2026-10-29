#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

void Display_Init(void);   /* call once before the main loop */
void ArmDrawFrame(int32_t theta_deg, int32_t phi_deg, uint8_t clawClosed);

#endif
