#include "LCD.h"

#include "CPU.h"
#include "Memory.h"

LCD::LCD(CPU* cpu, Memory* memory, ResourceHandle frameTexture, uint8_t* frameTextureData)
    : m_cpu(cpu)
	, m_memory(memory)
	, m_frameTexture(frameTexture)
	, m_frameTextureData(frameTextureData)
    , m_timerCounter(0)
	, m_currentLine(0)
{
}

LCD::~LCD()
{
}

void LCD::update(double deltaTimeSeconds)
{
	if (m_memory->read(0xFF44) != m_currentLine)
	{
		m_currentLine = 0;
	}

	uint8_t LCDStatusRegister = m_memory->read(0xFF41);
	uint8_t LYCLYInterruptEnable = (LCDStatusRegister & 0b01000000) >> 6;
	uint8_t OAMInterruptEnable = (LCDStatusRegister & 0b00100000) >> 5;
	uint8_t VBlankInterruptEnable = (LCDStatusRegister & 0b00010000) >> 4;
	uint8_t HBlankInterruptEnable = (LCDStatusRegister & 0b00001000) >> 3;

	double clockTicksToIncrement = CPU::FrequencyHz * deltaTimeSeconds;
	m_timerCounter += clockTicksToIncrement;

	switch (LCDStatusRegister & 3)
	{
		// OAM read mode, scanline active
	case 2:
		if (m_timerCounter >= 80)
		{
			// Enter scanline mode 3
			m_timerCounter = 0;
			m_memory->write(0xFF41, (m_memory->read(0xFF41) & 0b11111100) | 3);
		}
		break;

		// VRAM read mode, scanline active
		// Treat end of mode 3 as end of scanline
	case 3:
		if (m_timerCounter >= 172)
		{
			// Enter hblank
			m_timerCounter = 0;
			m_memory->write(0xFF41, (m_memory->read(0xFF41) & 0b11111100));
			if (HBlankInterruptEnable)
			{
				m_cpu->requestInterrupt(CPU::Interrupt::LCD_STAT);
			}

			writeScanlineToFrame();
		}
		break;

		// Hblank
	case 0:
		if (m_timerCounter >= 204)
		{
			m_timerCounter = 0;
			m_currentLine++;

			if (m_currentLine == 143)
			{
				// Enter vblank
				m_memory->write(0xFF41, (m_memory->read(0xFF41) & 0b11111100) | 1);
				if (VBlankInterruptEnable)
				{
					m_cpu->requestInterrupt(CPU::Interrupt::LCD_STAT);
					m_cpu->requestInterrupt(CPU::Interrupt::VBlank);
				}
				m_frameTexture.setNeedsCopyToGPU();
			}
			else
			{
				m_memory->write(0xFF41, (m_memory->read(0xFF41) & 0b11111100) | 2);
				if (OAMInterruptEnable)
				{
					m_cpu->requestInterrupt(CPU::Interrupt::LCD_STAT);
				}
			}
		}
		break;

		// Vblank (10 lines)
	case 1:
		if (m_timerCounter >= 4560)
		{
			m_timerCounter = 0;
			m_currentLine++;

			if (m_currentLine > 153)
			{
				// Restart scanning modes
				m_memory->write(0xFF41, (m_memory->read(0xFF41) & 0b11111100) | 2);
				if (OAMInterruptEnable)
				{
					m_cpu->requestInterrupt(CPU::Interrupt::LCD_STAT);
				}
				m_currentLine = 0;
			}
		}
		break;
	}

	// Update LY
	m_memory->write(0xFF44, m_currentLine);

	// Request interrupt if LYC==LY and is enabled
	if (m_memory->read(0xFF44) == m_memory->read(0xFF45))
	{
		m_memory->write(0xFF41, m_memory->read(0xFF41) | (1 << 2));
		if (LYCLYInterruptEnable)
		{
			m_cpu->requestInterrupt(CPU::Interrupt::LCD_STAT);
		}
	}
	else
	{
		m_memory->write(0xFF41, m_memory->read(0xFF41) & ~(1 << 2));
	}
}

void LCD::writeScanlineToFrame()
{
	if ((m_memory->read(0xFF40) & 0b10000000) == 0)
	{
		return;
	}

	// Read LCD control register
	uint8_t LCDC = m_memory->read(0xFF40);
	uint8_t LCDDisplayEnable = (LCDC & 128) >> 7;		// (0=Off, 1=On)
	uint8_t WindowTileMapSelect = (LCDC & 64) >> 6;		// (0=9800-9BFF, 1=9C00-9FFF)
	uint8_t WindowDisplayEnable = (LCDC & 32) >> 5;		// (0=Off, 1=On)
	uint8_t BGWindowTileDataSelect = (LCDC & 16) >> 4;	// (0=8800-97FF, 1=8000-8FFF)
	uint8_t BGTileMapDisplaySelect = (LCDC & 8) >> 3;	// (0=9800-9BFF, 1=9C00-9FFF)
	uint8_t SpriteSize = (LCDC & 4) >> 2;				// (0=8x8, 1=8x16)
	uint8_t SpriteDisplayEnable = (LCDC & 2) >> 1;		// (0=Off, 1=On)
	uint8_t BGDisplay = (LCDC & 1);						// (0=Off, 1=On)

	uint8_t SCX = m_memory->read(0xFF43);
	uint8_t SCY = m_memory->read(0xFF42);
	uint8_t paletteColors = m_memory->read(0xFF47);

	int bgmX = SCX;
	int bgmY = (SCY + m_currentLine) % 256;
	for (uint32_t i = 0; i < 144; i++)
	{
		for (uint32_t j = 0; j < 160; j++)
		{
			//uint8_t color = bgm[(i * 160 + j)]*255;
			uint8_t color = m_memory->read(0x9800 + (i * 160 + j));
			m_frameTextureData[(i * 160 + j) * 4] = color;
			m_frameTextureData[(i * 160 + j) * 4 + 1] = color;
			m_frameTextureData[(i * 160 + j) * 4 + 2] = color;
			m_frameTextureData[(j * 160 + j) * 4 + 3] = 0;
		}
	}
}
