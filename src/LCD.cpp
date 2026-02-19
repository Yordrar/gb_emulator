#include "LCD.h"

#include "Emulator.h"
#include "CPU.h"
#include "Memory.h"

#include <vector>
#include <algorithm>

static const LCD::RGB originalGBPalette[4] = { {0x9B, 0xBC, 0x0F}, {0x8B, 0xAC, 0x0F}, {0x30, 0x62, 0x30}, {0x0F, 0x38, 0x0F} };
static const LCD::RGB lospecPalette[4] = { {0xC7, 0xC6, 0xC6}, {0x7C, 0x6D, 0x80}, {0x38, 0x28, 0x43}, {0x00, 0x00, 0x00} };
static const LCD::RGB greyscalePalette[4] = { {255, 255, 255}, {168, 168, 168}, {84, 84, 84}, {0, 0, 0} };

static const LCD::RGB* sc_currentPalette = lospecPalette;

static const uint8_t sc_maxBrightness = 200;
static const LCD::RGB sc_white = { sc_maxBrightness, sc_maxBrightness, sc_maxBrightness };

LCD::LCD(CPU* cpu, Memory* memory, ResourceHandle frameTexture, uint8_t* frameTextureData)
    : m_cpu(cpu)
	, m_memory(memory)
	, m_frameTexture(frameTexture)
	, m_frameTextureData(frameTextureData)
    , m_timerCounter(0)
	, m_currentLine(-1)
{
}

LCD::~LCD()
{
}

void LCD::clearScreen()
{
	for (uint32_t i = 0; i < 144; i++)
	{
		fillScanlineWithColor(i, sc_white);
	}
}

void LCD::fillScanlineWithColor(uint8_t line, RGB color)
{
	for (uint32_t j = 0; j < 160; j++)
	{
		m_frameTextureData[(line * 160 + j) * 4] = color.r;
		m_frameTextureData[(line * 160 + j) * 4 + 1] = color.g;
		m_frameTextureData[(line * 160 + j) * 4 + 2] = color.b;
		m_frameTextureData[(line * 160 + j) * 4 + 3] = 1;
	}
}

void LCD::update(uint64_t cyclesToEmulate)
{
	if (m_memory->read(0xFF44) != m_currentLine && m_currentLine != -1)
	{
		m_currentLine = 0;
	}

	uint8_t LCDC = m_memory->read(0xFF40);
	uint8_t newIsDisplayEnabled = (LCDC & 128) >> 7;		// (0=Off, 1=On)
	if (m_isDisplayEnabled && !newIsDisplayEnabled)
	{
		// if the LCD was just disabled, clear LY=LYC and mode bits in STAT, set LY to 0 and clear the framebuffer
		m_memory->write(0xFF41, m_memory->read(0xFF41) & 0xF8);
		m_currentLine = 0;
		m_memory->write(0xFF44, m_currentLine);
		m_timerCounter = 0;
	}

	if (!m_isDisplayEnabled && newIsDisplayEnabled)
	{
		// if the LCD was just enabled, reset state to first scanline, clear the STAT interrupt and skip rendering the next frame
		m_memory->write(0xFF41, (m_memory->read(0xFF41) & 0b11111100) | 2);
		m_memory->write(0xFF0F, m_memory->read(0xFF0F) & ~(1 << CPU::Interrupt::LCD_STAT));
		m_skipNextFrame = true;
		m_timerCounter = 0;
		clearScreen();
	}

	m_isDisplayEnabled = newIsDisplayEnabled;

	if (!m_isDisplayEnabled)
	{
		return;
	}

	uint8_t LCDStatusRegister = m_memory->read(0xFF41);
	uint8_t LYCLYInterruptEnable = (LCDStatusRegister & 0b01000000) >> 6;
	uint8_t OAMInterruptEnable = (LCDStatusRegister & 0b00100000) >> 5;
	uint8_t VBlankInterruptEnable = (LCDStatusRegister & 0b00010000) >> 4;
	uint8_t HBlankInterruptEnable = (LCDStatusRegister & 0b00001000) >> 3;

	m_timerCounter += cyclesToEmulate;

	switch (LCDStatusRegister & 3)
	{
		// OAM read mode, scanline active
	case 2:
		if (m_timerCounter >= 80)
		{
			// Enter scanline mode 3
			m_timerCounter -= 80;
			m_memory->write(0xFF41, (m_memory->read(0xFF41) & 0b11111100) | 3);
		}
		break;

		// VRAM read mode, scanline active
		// Treat end of mode 3 as end of scanline
	case 3:
		if (m_timerCounter >= 172)
		{
			// Enter hblank
			m_timerCounter -= 172;
			m_memory->write(0xFF41, (m_memory->read(0xFF41) & 0b11111100));

			if (HBlankInterruptEnable)
			{
				m_cpu->requestInterrupt(CPU::Interrupt::LCD_STAT);
			}

			writeScanlineToFrame();

			if (m_currentLine >= 0 && m_currentLine <= 143)
			{
				m_memory->performHBlankDMATransfer();
			}
		}
		break;

		// Hblank
	case 0:
		if (m_timerCounter >= 204)
		{
			m_timerCounter -= 204;
			m_currentLine++;

			if (m_currentLine == 144)
			{
				// Enter vblank
				m_memory->write(0xFF41, (m_memory->read(0xFF41) & 0b11111100) | 1);

				m_cpu->requestInterrupt(CPU::Interrupt::VBlank);
				if (VBlankInterruptEnable)
				{
					m_cpu->requestInterrupt(CPU::Interrupt::LCD_STAT);
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
		if (m_timerCounter >= 456)
		{
			m_timerCounter -= 456;
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
				m_skipNextFrame = false;
			}
		}
		break;
	}

	// Update LY
	m_memory->write(0xFF44, m_currentLine);

	// Request interrupt if LYC==LY and is enabled
	if (m_currentLine == m_memory->read(0xFF45))
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
	if (m_currentLine >= 144 || m_skipNextFrame) return;

	// Read LCD control register
	uint8_t LCDC = m_memory->read(0xFF40);
	uint8_t LCDDisplayEnable = (LCDC & 128) >> 7;		// (0=Off, 1=On)
	uint8_t WindowTileMapSelect = (LCDC & 64) >> 6;		// (0=9800-9BFF, 1=9C00-9FFF)
	uint8_t WindowDisplayEnable = (LCDC & 32) >> 5;		// (0=Off, 1=On)
	uint8_t BGWindowTileDataSelect = (LCDC & 16) >> 4;	// (0=8800-97FF, 1=8000-8FFF)
	uint8_t BGTileMapDisplaySelect = (LCDC & 8) >> 3;	// (0=9800-9BFF, 1=9C00-9FFF)
	uint8_t SpriteSize = (LCDC & 4) >> 2;				// (0=8x8, 1=8x16)
	uint8_t SpriteDisplayEnable = (LCDC & 2) >> 1;		// (0=Off, 1=On)
	uint8_t BGDisplayEnable = (LCDC & 1);				// (0=Off, 1=On)
	uint8_t BGDisplayPriority = (LCDC & 1);				// CGB-only (0=BG/Window lose prio, 1=Prio is as normal)

	if (LCDDisplayEnable == 0)
	{
		fillScanlineWithColor(m_currentLine, Emulator::isCGBMode() ? sc_white : sc_currentPalette[0]);
		return;
	}

	if (BGDisplayEnable || Emulator::isCGBMode())
	{
		uint8_t WX = m_memory->read(0xFF4B) - 7;
		uint8_t WY = m_memory->read(0xFF4A);
		uint8_t SCX = m_memory->read(0xFF43);
		uint8_t SCY = m_memory->read(0xFF42);
		uint8_t paletteColors = m_memory->read(0xFF47);
		uint16_t beginBGTileMap = BGTileMapDisplaySelect ? 0x9C00 : 0x9800;
		uint16_t beginWindowTileMap = WindowTileMapSelect ? 0x9C00 : 0x9800;
		uint16_t beginBGWindowTileData = BGWindowTileDataSelect ? 0x8000 : 0x8800;

		int bgmX = SCX;
		int bgmY = (SCY + m_currentLine) % 256;
		int tileMapX = bgmX / 8;
		int tileMapY = (bgmY / 8) % 32;
		int tileMapOffset = (tileMapY * 32) + tileMapX;
		int tileOffsetX = 7 - (bgmX % 8);
		int tileOffsetY = bgmY % 8;
		for (uint32_t j = 0; j < 160; j++)
		{
			uint16_t beginTileMap = 0;
			if (WindowDisplayEnable && j >= WX && m_currentLine >= WY)
			{
				beginTileMap = beginWindowTileMap;
				tileMapX = ((j - WX) / 8) % 32;
				tileMapY = ((m_currentLine - WY) / 8) % 32;
				tileMapOffset = (tileMapY * 32) + tileMapX;
				tileOffsetX = 7 - ((j - WX) % 8);
				tileOffsetY = (m_currentLine - WY) % 8;
			}
			else
			{
				beginTileMap = beginBGTileMap;
				tileMapX = ((bgmX + j) / 8) % 32;
				tileMapOffset = (tileMapY * 32) + tileMapX;
				tileOffsetX = 7 - ((bgmX + j) % 8);
				tileOffsetY = bgmY % 8;
			}

			// CGB BG Map Attributes
			uint8_t attr = Emulator::isCGBMode() ? m_memory->readFromVramBank(beginTileMap + tileMapOffset, 1) : 0;
			uint8_t priority = (attr >> 7);
			uint8_t yFlip = (attr >> 6) & 1;
			uint8_t xFlip = (attr >> 5) & 1;
			uint8_t bank = Emulator::isCGBMode() ? (attr >> 3) & 1 : 0;
			uint8_t colorPaletteIdx = (attr & 7);

			if (yFlip)
			{
				tileOffsetY = 7 - tileOffsetY;
			}
			if (xFlip)
			{
				tileOffsetX = 7 - tileOffsetX;
			}

			m_priorityMap[(m_currentLine * 160 + j)] = (m_priorityMap[(m_currentLine * 160 + j)] & 0x2) | (BGDisplayPriority << 2) | (priority);

			uint16_t tileIdx = 0;
			if (!BGWindowTileDataSelect)
			{
				uint8_t signedTileIdx = m_memory->readFromVramBank(beginTileMap + tileMapOffset, 0);
				if (signedTileIdx & 0x80)
				{
					tileIdx = signedTileIdx - 0x80;
				}
				else
				{
					tileIdx = signedTileIdx + 0x80;
				}
			}
			else
			{
				tileIdx = m_memory->readFromVramBank(beginTileMap + tileMapOffset, 0);
			}

			uint8_t tileLSB = m_memory->readFromVramBank(beginBGWindowTileData + (tileIdx << 4) + (tileOffsetY << 1), bank);
			uint8_t tileMSB = m_memory->readFromVramBank(beginBGWindowTileData + (tileIdx << 4) + (tileOffsetY << 1) + 1, bank);
			uint8_t paletteIdx = (((tileMSB & (1 << tileOffsetX)) >> tileOffsetX) << 1) | (((tileLSB & (1 << tileOffsetX)) >> tileOffsetX));
			m_BGColorIndex[(m_currentLine * 160 + j)] = paletteIdx;
			if(Emulator::isCGBMode())
			{
				uint16_t packedColor = m_memory->m_BGColorPaletteRam[(colorPaletteIdx * 4) + paletteIdx];
				m_frameTextureData[(m_currentLine * 160 + j) * 4] = (uint8_t)round(((packedColor & 0x1F) / 31.0) * sc_maxBrightness);
				m_frameTextureData[(m_currentLine * 160 + j) * 4 + 1] = (uint8_t)round((((packedColor >> 5) & 0x1F) / 31.0) * sc_maxBrightness);
				m_frameTextureData[(m_currentLine * 160 + j) * 4 + 2] = (uint8_t)round((((packedColor >> 10) & 0x1F) / 31.0) * sc_maxBrightness);
				m_frameTextureData[(m_currentLine * 160 + j) * 4 + 3] = 1;
			}
			else
			{
				uint8_t paletteColor = (paletteColors >> (paletteIdx * 2)) & 3;
				m_frameTextureData[(m_currentLine * 160 + j) * 4] = sc_currentPalette[paletteColor].r;
				m_frameTextureData[(m_currentLine * 160 + j) * 4 + 1] = sc_currentPalette[paletteColor].g;
				m_frameTextureData[(m_currentLine * 160 + j) * 4 + 2] = sc_currentPalette[paletteColor].b;
				m_frameTextureData[(m_currentLine * 160 + j) * 4 + 3] = 1;
			}
		}
	}
	else
	{
		if (!Emulator::isCGBMode())
		{
			for (uint32_t j = 0; j < 160; j++)
			{
				m_frameTextureData[(m_currentLine * 160 + j) * 4] = sc_currentPalette[0].r;
				m_frameTextureData[(m_currentLine * 160 + j) * 4 + 1] = sc_currentPalette[0].g;
				m_frameTextureData[(m_currentLine * 160 + j) * 4 + 2] = sc_currentPalette[0].b;
				m_frameTextureData[(m_currentLine * 160 + j) * 4 + 3] = 1;
			}
		}
	}

	uint8_t spritesDrawnThisScanline = 0;
	if (SpriteDisplayEnable)
	{
		m_spritesToDraw.clear();
		// Selection priority, first 10 in mem that can be drawn are selected
		for (int i = 0; i < 40; i++)
		{
			Sprite sprite;
			sprite.spriteY = m_memory->read(0xFE00 + (i * 4));
			sprite.spriteX = m_memory->read(0xFE00 + (i * 4) + 1);
			sprite.tileIndex = m_memory->read(0xFE00 + (i * 4) + 2);
			sprite.attributes = m_memory->read(0xFE00 + (i * 4) + 3);
			sprite.locationInOAM = i;

			if ((sprite.spriteY - 16) <= m_currentLine &&
				(sprite.spriteY - 16 + (SpriteSize ? 16 : 8)) > m_currentLine)
			{
				m_spritesToDraw.push_back(sprite);
			}
			if (m_spritesToDraw.size() >= 10)
			{
				break;
			}
		}
		// Drawing priority
		std::sort(m_spritesToDraw.begin(), m_spritesToDraw.end(), [&](Sprite const& a, Sprite const& b)
			{
				if (!Emulator::isCGBMode())
				{
					if (a.spriteX < b.spriteX) return true;
					if (a.spriteX == b.spriteX && a.locationInOAM < b.locationInOAM) return true;
				}
				else
				{
					if (a.locationInOAM < b.locationInOAM) return true;
				}
				return false;
			});
		for (int rowPixel = 0; rowPixel < 160; ++rowPixel)
		{
			for (int i = (int)m_spritesToDraw.size() - 1; i >= 0; --i)
			{
				if (m_spritesToDraw[i].spriteX - 8 <= rowPixel && m_spritesToDraw[i].spriteX > rowPixel)
				{
					uint8_t currentBGRed = m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4];
					uint8_t currentBGGreen = m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 1];
					uint8_t currentBGBlue = m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 2];

					uint8_t spriteY = m_spritesToDraw[i].spriteY;
					uint8_t spriteX = m_spritesToDraw[i].spriteX;
					uint8_t spriteTile = m_spritesToDraw[i].tileIndex;
					uint8_t spriteAttributes = m_spritesToDraw[i].attributes;
					uint8_t OBJtoBGPriority = (spriteAttributes & 128) >> 7;
					uint8_t spriteYFlip = (spriteAttributes & 64) >> 6;
					uint8_t spriteXFlip = (spriteAttributes & 32) >> 5;
					uint8_t paletteNumber = (spriteAttributes & 16) >> 4;
					uint8_t bank = Emulator::isCGBMode() ? (spriteAttributes & 8) >> 3 : 0;
					uint8_t colorPaletteIdx = (spriteAttributes & 7);
					uint8_t paletteColors = paletteNumber ? m_memory->read(0xFF49) : m_memory->read(0xFF48);
					uint16_t beginSpriteTileData = 0x8000;

					m_priorityMap[(m_currentLine * 160 + rowPixel)] = (m_priorityMap[(m_currentLine * 160 + rowPixel)] & 0x5) | (OBJtoBGPriority << 1);

					int spriteOffsetY = m_currentLine - (spriteY - 16);
					if (spriteYFlip)
					{
						if (SpriteSize)
						{
							spriteOffsetY = 15 - spriteOffsetY;
						}
						else
						{
							spriteOffsetY = 7 - spriteOffsetY;
						}
					}
					if (SpriteSize) // if 8x16 mode, adjust the sprite tile number
					{
						if (spriteOffsetY < 8)
						{
							spriteTile &= 0xFE;
						}
						else
						{
							spriteOffsetY -= 8;
							spriteTile |= 0x01;
						}
					}
					int spriteOffsetX = 7 - (rowPixel - (spriteX - 8));
					if (spriteXFlip)
					{
						spriteOffsetX = 7 - spriteOffsetX;
					}

					uint8_t spriteLSB = m_memory->readFromVramBank(beginSpriteTileData + (spriteTile << 4) + (spriteOffsetY << 1), bank);
					uint8_t spriteMSB = m_memory->readFromVramBank(beginSpriteTileData + (spriteTile << 4) + (spriteOffsetY << 1) + 1, bank);
					uint8_t paletteIdx = (((spriteMSB & (1 << spriteOffsetX)) >> spriteOffsetX) << 1) | (((spriteLSB & (1 << spriteOffsetX)) >> spriteOffsetX));
					if (paletteIdx != 0)
					{
						if (Emulator::isCGBMode())
						{
							uint8_t prioMapValue = m_priorityMap[(m_currentLine * 160 + rowPixel)];
							if (prioMapValue <= 4 || (prioMapValue > 4 && m_BGColorIndex[(m_currentLine * 160 + rowPixel)] == 0))
							{
								uint16_t packedColor = m_memory->m_OBJColorPaletteRam[(colorPaletteIdx * 4) + paletteIdx];
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4] = (uint8_t)round(((packedColor & 0x1F) / 31.0) * sc_maxBrightness);
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 1] = (uint8_t)round((((packedColor >> 5) & 0x1F) / 31.0) * sc_maxBrightness);
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 2] = (uint8_t)round((((packedColor >> 10) & 0x1F) / 31.0) * sc_maxBrightness);
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 3] = 1;
							}
							else
							{
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4] = currentBGRed;
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 1] = currentBGGreen;
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 2] = currentBGBlue;
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 3] = 1;
							}
						}
						else
						{
							if (!OBJtoBGPriority
								||
								(OBJtoBGPriority && m_BGColorIndex[(m_currentLine * 160 + rowPixel)] == 0))
							{
								uint8_t paletteColor = (paletteColors >> (paletteIdx * 2)) & 3;
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4] = sc_currentPalette[paletteColor].r;
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 1] = sc_currentPalette[paletteColor].g;
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 2] = sc_currentPalette[paletteColor].b;
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 3] = 1;
							}
							else
							{
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4] = currentBGRed;
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 1] = currentBGGreen;
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 2] = currentBGBlue;
								m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 3] = 1;
							}
						}
					}
				}
			}
		}
	}
}
