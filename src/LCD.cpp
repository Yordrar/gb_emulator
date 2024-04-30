#include "LCD.h"

#include "CPU.h"
#include "Memory.h"

LCD::LCD(CPU* cpu, Memory* memory, ResourceHandle frameTexture)
    : m_cpu(cpu)
	, m_memory(memory)
	, m_frameTexture(frameTexture)
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

			// TODO Write a scanline to the framebuffer
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
				// TODO After the last hblank, push the screen data to canvas
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
		if (m_timerCounter >= 456)
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
