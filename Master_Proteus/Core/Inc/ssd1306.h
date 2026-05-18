#ifndef __SSD1306_H__
#define __SSD1306_H__

#include "main.h"

void SSD1306_Init(void);
void SSD1306_Clear(void);
void SSD1306_ShowStr(uint8_t Line, uint8_t Col, const char *str);
void SSD1306_ShowInt(uint8_t Line, uint8_t Col, int32_t val);

#endif
