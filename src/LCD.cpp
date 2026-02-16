#include "LCD.h"

#include "Emulator.h"
#include "CPU.h"
#include "Memory.h"

#include <vector>
#include <algorithm>

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

void LCD::update(uint64_t cyclesToEmulate)
{
	if (m_memory->read(0xFF44) != m_currentLine && m_currentLine != -1)
	{
		m_currentLine = 0;
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

struct RGB
{
	uint8_t r, g, b;
};
static const RGB originalGBPalette[4] = { {0x9B, 0xBC, 0x0F}, {0x8B, 0xAC, 0x0F}, {0x30, 0x62, 0x30}, {0x0F, 0x38, 0x0F} };
static const RGB lospecPalette[4] = { {0xC7, 0xC6, 0xC6}, {0x7C, 0x6D, 0x80}, {0x38, 0x28, 0x43}, {0x00, 0x00, 0x00} };
static const RGB greyscalePalette[4] = { {255, 255, 255}, {168, 168, 168}, {84, 84, 84}, {0, 0, 0} };

static const RGB* currentPalette = lospecPalette;

struct Sprite
{
	int spriteY, spriteX, tileIndex, attributes, locationInOAM;
};
void LCD::writeScanlineToFrame()
{
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

	if (LCDDisplayEnable == 0)
	{
		return;
	}

	if (BGDisplayEnable)
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
		int tileMapY = bgmY / 8;
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
				tileMapY = (m_currentLine - WY) / 8;
				tileOffsetX = 7 - ((j - WX) % 8);
				tileOffsetY = (m_currentLine - WY) % 8;
				tileMapOffset = (tileMapY * 32) + tileMapX;
			}
			else
			{
				beginTileMap = beginBGTileMap;
				tileMapX = ((bgmX + j) / 8) % 32;
				tileOffsetX = 7 - ((bgmX + j) % 8);
				tileMapOffset = (tileMapY * 32) + tileMapX;
			}

			uint8_t tileIdx = 0;
			if (!BGWindowTileDataSelect)
			{
				uint8_t signedTileIdx = m_memory->read(beginTileMap + tileMapOffset);
				tileIdx = (signedTileIdx - 0x80);
			}
			else
			{
				tileIdx = m_memory->read(beginTileMap + tileMapOffset);
			}
			uint8_t tileLSB = m_memory->read(beginBGWindowTileData + (tileIdx << 4) + (tileOffsetY << 1));
			uint8_t tileMSB = m_memory->read(beginBGWindowTileData + (tileIdx << 4) + (tileOffsetY << 1) + 1);
			uint8_t paletteIdx = (((tileMSB & (1 << tileOffsetX)) >> tileOffsetX) << 1) | (((tileLSB & (1 << tileOffsetX)) >> tileOffsetX));
			uint8_t paletteColor = (paletteColors >> (paletteIdx * 2)) & 3;
			m_frameTextureData[(m_currentLine * 160 + j) * 4] = currentPalette[paletteColor].r;
			m_frameTextureData[(m_currentLine * 160 + j) * 4 + 1] = currentPalette[paletteColor].g;
			m_frameTextureData[(m_currentLine * 160 + j) * 4 + 2] = currentPalette[paletteColor].b;
			m_frameTextureData[(m_currentLine * 160 + j) * 4 + 3] = 1;
		}
	}
	else
	{
		for (uint32_t j = 0; j < 160; j++)
		{
			m_frameTextureData[(m_currentLine * 160 + j) * 4] = currentPalette[0].r;
			m_frameTextureData[(m_currentLine * 160 + j) * 4 + 1] = currentPalette[0].g;
			m_frameTextureData[(m_currentLine * 160 + j) * 4 + 2] = currentPalette[0].b;
			m_frameTextureData[(m_currentLine * 160 + j) * 4 + 3] = 1;
		}
	}

	uint8_t spritesDrawnThisScanline = 0;
	if (SpriteDisplayEnable)
	{
		// Put all sprites into a structure to filter them easier later
		std::vector<Sprite> sprites;
		for (int i = 0; i < 40; i++)
		{
			Sprite sprite;
			sprite.spriteY = m_memory->read(0xFE00 + (i * 4));
			sprite.spriteX = m_memory->read(0xFE00 + (i * 4) + 1);
			sprite.tileIndex = m_memory->read(0xFE00 + (i * 4) + 2);
			sprite.attributes = m_memory->read(0xFE00 + (i * 4) + 3);
			sprite.locationInOAM = i;
			sprites.push_back(sprite);
		}
		std::vector<Sprite> spritesToDraw;
		// Selection priority, first 10 in mem that can be drawn are selected
		for (int i = 0; i < 40; i++)
		{
			Sprite const& sprite = sprites[i];
			if ((sprite.spriteY - 16) <= m_currentLine &&
				(sprite.spriteY - 16 + (SpriteSize ? 16 : 8)) > m_currentLine)
			{
				spritesToDraw.push_back(sprite);
			}
			if (spritesToDraw.size() >= 10)
			{
				break;
			}
		}
		// Drawing priority
		std::sort(spritesToDraw.begin(), spritesToDraw.end(), [](Sprite const& a, Sprite const& b)
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
			std::vector<Sprite> spritesInThisPixel;
			for (int i = 0; i < spritesToDraw.size(); ++i)
			{
				if (spritesToDraw[i].spriteX - 8 <= rowPixel && spritesToDraw[i].spriteX > rowPixel)
				{
					spritesInThisPixel.push_back(spritesToDraw[i]);
				}
			}

			uint8_t currentBGRed = m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4];
			uint8_t currentBGGreen = m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 1];
			uint8_t currentBGBlue = m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 2];

			for (int i = spritesInThisPixel.size() - 1; i >= 0; i--)
			{
				uint8_t spriteY = spritesInThisPixel[i].spriteY;
				uint8_t spriteX = spritesInThisPixel[i].spriteX;
				uint8_t spriteTile = spritesInThisPixel[i].tileIndex;
				uint8_t spriteAttributes = spritesInThisPixel[i].attributes;
				uint8_t OBJtoBGPriority = (spriteAttributes & 128) >> 7;
				uint8_t spriteYFlip = (spriteAttributes & 64) >> 6;
				uint8_t spriteXFlip = (spriteAttributes & 32) >> 5;
				uint8_t paletteNumber = (spriteAttributes & 16) >> 4;
				uint8_t paletteColors = paletteNumber ? m_memory->read(0xFF49) : m_memory->read(0xFF48);
				uint16_t beginSpriteTileData = 0x8000;

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
				if (!OBJtoBGPriority
					||
					(OBJtoBGPriority && m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4] == currentPalette[m_memory->read(0xFF47)&3].r))
				{
					uint8_t spriteLSB = m_memory->read(beginSpriteTileData + (spriteTile << 4) + (spriteOffsetY << 1));
					uint8_t spriteMSB = m_memory->read(beginSpriteTileData + (spriteTile << 4) + (spriteOffsetY << 1) + 1);
					uint8_t paletteIdx = (((spriteMSB & (1 << spriteOffsetX)) >> spriteOffsetX) << 1) | (((spriteLSB & (1 << spriteOffsetX)) >> spriteOffsetX));
					uint8_t paletteColor = (paletteColors >> (paletteIdx * 2)) & 3;
					if (paletteIdx != 0)
					{
						m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4] = currentPalette[paletteColor].r;
						m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 1] = currentPalette[paletteColor].g;
						m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 2] = currentPalette[paletteColor].b;
						m_frameTextureData[(m_currentLine * 160 + rowPixel) * 4 + 3] = 1;
					}
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
