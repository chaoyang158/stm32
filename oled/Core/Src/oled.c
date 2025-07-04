#include "oled.h"
#include "oledfont.h"
#include <string.h>

#define OLED_ADDR 0x78 // OLED I2C 7位地址: 0x3C << 1 = 0x78

extern I2C_HandleTypeDef hi2c1;
uint8_t OLED_GRAM[8][128];

// OLED初始化命令
const uint8_t CMD_Data[] = {
    0xAE, 0x00, 0x10, 0x40, 0xB0, 0x81, 0xFF, 0xA1, 0xA6, 0xA8, 0x3F,
    0xC8, 0xD3, 0x00, 0xD5, 0x80, 0xD8, 0x05, 0xD9, 0xF1, 0xDA, 0x12,
    0xD8, 0x30, 0x8D, 0x14, 0xAF};

// 写命令
void OLED_WR_CMD(uint8_t cmd)
{
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, &cmd, 1, HAL_MAX_DELAY);
}

// 写数据
void OLED_WR_DATA(uint8_t data)
{
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY);
}

// 批量初始化命令
void WriteCmd(void)
{
  for (uint8_t i = 0; i < sizeof(CMD_Data); i++)
  {
    OLED_WR_CMD(CMD_Data[i]);
  }
}

void OLED_Init(void)
{
  HAL_Delay(100);
  WriteCmd();
  OLED_Clear();
  OLED_Refresh();
}

void OLED_Display_On(void)
{
  OLED_WR_CMD(0x8D);
  OLED_WR_CMD(0x14);
  OLED_WR_CMD(0xAF);
}

void OLED_Display_Off(void)
{
  OLED_WR_CMD(0x8D);
  OLED_WR_CMD(0x10);
  OLED_WR_CMD(0xAE);
}

void OLED_Clear(void)
{
  memset(OLED_GRAM, 0x00, sizeof(OLED_GRAM));
}

// 更新位置
void OLED_Set_Pos(uint8_t x, uint8_t y)
{
  OLED_WR_CMD(0xB0 + y);
  OLED_WR_CMD(((x & 0xF0) >> 4) | 0x10);
  OLED_WR_CMD(x & 0x0F);
}

// 普通非DMA刷新
void OLED_Refresh(void)
{
  for (uint8_t page = 0; page < 8; page++)
  {
    OLED_Set_Pos(0, page);
    HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, OLED_GRAM[page], 128, HAL_MAX_DELAY);
  }
}

// DMA刷新
void OLED_Refresh_DMA(void)
{
  for (uint8_t page = 0; page < 8; page++)
  {
    uint8_t cmd[] = {0x00, (uint8_t)(0xB0 + page), 0x00, 0x10};
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, cmd, sizeof(cmd), HAL_MAX_DELAY);
    HAL_I2C_Mem_Write_DMA(&hi2c1, OLED_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, OLED_GRAM[page], 128);

    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY);
  }
}

// 写入GRAM
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t)
{
  if (x > 127 || y > 63)
    return;
  uint8_t page = y / 8;
  uint8_t bit = y % 8;
  if (t)
    OLED_GRAM[page][x] |= (1 << bit);
  else
    OLED_GRAM[page][x] &= ~(1 << bit);
}

unsigned int oled_pow(uint8_t m, uint8_t n)
{
  unsigned int result = 1;
  while (n--)
    result *= m;
  return result;
}

void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size)
{
  uint8_t c = chr - ' ';
  if (x > 127 - 1)
  {
    x = 0;
    y += 2;
  }
  if (size == 16)
  {
    for (uint8_t i = 0; i < 8; i++)
      OLED_GRAM[y][x + i] = F8x16[c * 16 + i];
    for (uint8_t i = 0; i < 8; i++)
      OLED_GRAM[y + 1][x + i] = F8x16[c * 16 + i + 8];
  }
  else
  {
    for (uint8_t i = 0; i < 6; i++)
      OLED_GRAM[y][x + i] = F6x8[c][i];
  }
}

void OLED_ShowNum(uint8_t x, uint8_t y, unsigned int num, uint8_t len, uint8_t size)
{
  uint8_t t, temp, enshow = 0;
  for (t = 0; t < len; t++)
  {
    temp = (num / oled_pow(10, len - t - 1)) % 10;
    if (enshow == 0 && t < len - 1)
    {
      if (temp == 0)
      {
        OLED_ShowChar(x + (size / 2) * t, y, ' ', size);
        continue;
      }
      else
        enshow = 1;
    }
    OLED_ShowChar(x + (size / 2) * t, y, temp + '0', size);
  }
}

void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_t size)
{
  uint8_t j = 0;
  while (chr[j] != '\0')
  {
    OLED_ShowChar(x, y, chr[j], size);
    x += (size == 16) ? 8 : 6;
    if (x > 120)
    {
      x = 0;
      y += 2;
    }
    j++;
  }
}

extern uint8_t **Hzk;

void OLED_ShowCHinese(uint8_t x, uint8_t y, uint8_t no)
{
  for (uint8_t i = 0; i < 16; i++)
    OLED_GRAM[y][x + i] = Hzk[2 * no][i];
  for (uint8_t i = 0; i < 16; i++)
    OLED_GRAM[y + 1][x + i] = Hzk[2 * no + 1][i];
}
