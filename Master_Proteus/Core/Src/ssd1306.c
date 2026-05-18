#include "ssd1306.h"
#include "OLED_Font.h"

/* PB6=SCL, PB7=SDA, direct register access */
#define SCL_PIN  GPIO_PIN_6
#define SDA_PIN  GPIO_PIN_7

static void SCL_H(void) { GPIOB->BSRR = SCL_PIN; }
static void SCL_L(void) { GPIOB->BRR  = SCL_PIN; }
static void SDA_H(void) { GPIOB->BSRR = SDA_PIN; }
static void SDA_L(void) { GPIOB->BRR  = SDA_PIN; }

static void I2C_Start(void) {
  SDA_H(); SCL_H();
  SDA_L(); SCL_L();
}
static void I2C_Stop(void) {
  SDA_L(); SCL_H();
  SDA_H();
}
static void I2C_SendByte(uint8_t d) {
  uint8_t i;
  for (i = 0; i < 8; i++) {
    if (d & (0x80 >> i)) SDA_H(); else SDA_L();
    SCL_H();
    SCL_L();
  }
  SCL_H();
  SCL_L();
}
static void OLED_WriteCmd(uint8_t c) {
  I2C_Start(); I2C_SendByte(0x78); I2C_SendByte(0x00); I2C_SendByte(c); I2C_Stop();
}
static void OLED_WriteData(uint8_t d) {
  I2C_Start(); I2C_SendByte(0x78); I2C_SendByte(0x40); I2C_SendByte(d); I2C_Stop();
}
static void OLED_SetCursor(uint8_t Y, uint8_t X) {
  OLED_WriteCmd(0xB0 | Y);
  OLED_WriteCmd(0x10 | ((X & 0xF0) >> 4));
  OLED_WriteCmd(0x00 | (X & 0x0F));
}

void SSD1306_Init(void) {
  uint32_t i, j;
  for (i = 0; i < 500; i++)
    for (j = 0; j < 500; j++);

  /* Configure PB6/PB7 as open-drain output (same as StdPeriph GPIO_Mode_Out_OD) */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIOB->CRL &= ~(0xFF << 24);  /* clear PB6/PB7 config */
  GPIOB->CRL |= (0x77 << 24);   /* MODE=11(50MHz), CNF=01(open-drain) for both */

  SCL_H(); SDA_H();

  OLED_WriteCmd(0xAE); OLED_WriteCmd(0xD5); OLED_WriteCmd(0x80);
  OLED_WriteCmd(0xA8); OLED_WriteCmd(0x3F);
  OLED_WriteCmd(0xD3); OLED_WriteCmd(0x00);
  OLED_WriteCmd(0x40);
  OLED_WriteCmd(0xA1); OLED_WriteCmd(0xC8);
  OLED_WriteCmd(0xDA); OLED_WriteCmd(0x12);
  OLED_WriteCmd(0x81); OLED_WriteCmd(0xCF);
  OLED_WriteCmd(0xD9); OLED_WriteCmd(0xF1);
  OLED_WriteCmd(0xDB); OLED_WriteCmd(0x30);
  OLED_WriteCmd(0xA4); OLED_WriteCmd(0xA6);
  OLED_WriteCmd(0x8D); OLED_WriteCmd(0x14);
  OLED_WriteCmd(0xAF);
  SSD1306_Clear();
}

void SSD1306_Clear(void) {
  uint8_t i, j;
  for (j = 0; j < 8; j++) {
    OLED_WriteCmd(0xB0 + j);
    OLED_WriteCmd(0x00); OLED_WriteCmd(0x10);
    for (i = 0; i < 128; i++) OLED_WriteData(0x00);
  }
}

void SSD1306_ShowStr(uint8_t Line, uint8_t Col, const char *str) {
  uint8_t i;
  while (*str) {
    uint8_t c = *str;
    if (c < ' ' || c > '~') c = ' ';
    c -= ' ';
    OLED_SetCursor((Line - 1) * 2, (Col - 1) * 8);
    for (i = 0; i < 8; i++) OLED_WriteData(OLED_F8x16[c][i]);
    OLED_SetCursor((Line - 1) * 2 + 1, (Col - 1) * 8);
    for (i = 0; i < 8; i++) OLED_WriteData(OLED_F8x16[c][i + 8]);
    Col++; str++;
  }
}

void SSD1306_ShowInt(uint8_t Line, uint8_t Col, int32_t val) {
  char b[12]; int i = 0, neg = 0, j;
  if (val < 0) { neg = 1; val = -val; }
  if (val == 0) b[i++] = '0';
  else while (val) { b[i++] = '0' + (val % 10); val /= 10; }
  if (neg) b[i++] = '-';
  for (j = 0; j < i / 2; j++) { char t = b[j]; b[j] = b[i-1-j]; b[i-1-j] = t; }
  b[i] = 0;
  SSD1306_ShowStr(Line, Col, b);
}
