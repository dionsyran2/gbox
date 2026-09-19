#pragma once
#include <stdint.h>

#define PPU_OAM_MEM_START           0xFE00
#define PPU_OAM_MEM_END             0xFE9F

#define PPU_VRAM_MEM_START          0x8000
#define PPU_VRAM_MEM_END            0x9FFF

#define PPU_OAM_SIZE                ((PPU_OAM_MEM_END - PPU_OAM_MEM_START) + 1)
#define PPU_VRAM_SIZE               ((PPU_VRAM_MEM_END - PPU_VRAM_MEM_START) + 1)


#define PPU_REG_LCDC                0xFF40
#define PPU_REG_STAT                0xFF41
#define PPU_REG_SCY                 0xFF42
#define PPU_REG_SCX                 0xFF43
#define PPU_REG_LY                  0xFF44 // Read Only
#define PPU_REG_LYC                 0xFF45
#define PPU_REG_DMA                 0xFF46
#define PPU_REG_BGP                 0xFF47
#define PPU_REG_OBP0                0xFF48
#define PPU_REG_OBP1                0xFF49
#define PPU_REG_WY                  0xFF4A
#define PPU_REG_WX                  0xFF4B

#define PPU_LCDC_BIT_BG_EN          0x01
#define PPU_LCDC_BIT_OBJ_EN         0x02
#define PPU_LCDC_BIT_OBJ_SZ         0x04
#define PPU_LCDC_BIT_BG_TILE        0x08
#define PPU_LCDC_BIT_WIN_EN         0x20
#define PPU_LCDC_BIT_WIN_TILE       0x40
#define PPU_LCDC_BIT_LCD_EN         0x80

#define PPU_STAT_BIT_LYC_EQ_LY      (1 << 2);

enum PPUMode {
    MODE_HBLANK = 0,
    MODE_VBLANK = 1,
    MODE_OAM    = 2,
    MODE_DRAW   = 3
};

struct ppu_t {
    int dot_clock = 0;
    int pushed_pixels = 0;

    int bg_color_id = 0;

    bool window_visible_this_line = false;
    int window_lc = 0;

    int sprite_count = 0;
    int sprite_indices[10] = { 0 }; // Indices found during oam scan

    bool screen_cleared = false;

    // REGISTERS
    uint8_t lcd_control = 0; // PPU_REG_LCDC
    uint8_t lcd_status  = 0; // PPU_REG_STAT
    uint8_t ly = 0; // PPU_REG_LY
    uint8_t lyc = 0; // PPU_REG_LYC

    uint8_t bgp = 0xFC; // PPU_REG_BGP

    uint8_t scy = 0; // PPU_REG_SCY
    uint8_t scx = 0; // PPU_REG_SCX

    uint8_t wy = 0; // PPU_REG_WY
    uint8_t wx = 0; // PPU_REG_WX

    uint8_t obp0 = 0xFC; // PPU_REG_OBP0
    uint8_t obp1 = 0xFC; // PPU_REG_OBP1



    uint8_t oam[PPU_OAM_SIZE] = { 0 };
    uint8_t vram[PPU_VRAM_SIZE] = { 0 };

    void oam_scan();
    void tick_pixel_fifo();

    void draw_bg();
    void draw_win();
    void draw_sprite();

    void update(int cycles);
};


extern ppu_t ppu;