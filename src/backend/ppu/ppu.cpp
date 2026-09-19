#include <ppu/ppu.h>
#include <memory/memory.h>
#include <common.h>
#include <cstring>

ppu_t ppu = { 0 };

uint32_t colors[4] = { 0xFFE0F8D0, 0xFF88C070,   0xFF346856, 0xFF19332A };

void ppu_t::update(int cycles) {
    if ((this->lcd_control & PPU_LCDC_BIT_LCD_EN) == 0) {
        // PPU - LCD Disabled
        this->dot_clock = 0;
        this->pushed_pixels = 0;

        this->ly = 0;
        this->lcd_status &= 0xFC; // Clear the bottom 2 bits

        if (!screen_cleared) {
            screen_cleared = true;
            memset(g_state.screen, colors[0], sizeof(g_state.screen));
            g_state.frame_ready.store(true);
        }
        return;
    }

    screen_cleared = false;

    while (cycles > 0) {
        this->dot_clock++;
        cycles--;

        uint8_t current_mode = this->lcd_status & 0x03;
        uint8_t next_mode = current_mode;

        if (ly >= 144) {
            next_mode = MODE_VBLANK;
        } else {
            if (this->dot_clock <= 80) {
                next_mode = MODE_OAM;

                if (this->dot_clock == 80) {
                    this->oam_scan();
                }
            } else if (this->pushed_pixels < 160){
                next_mode = MODE_DRAW;

                this->tick_pixel_fifo();
            } else {
                next_mode = MODE_HBLANK;
            }
        }

        // Handle precise mode transitions and STAT interrupts
        if (next_mode != current_mode) {
            bool request_stat_interrupt = false;
            this->lcd_status = (this->lcd_status & 0xFC) | next_mode;

            if (next_mode == MODE_HBLANK && (this->lcd_status & 0x08)) request_stat_interrupt = true;
            if (next_mode == MODE_VBLANK && (this->lcd_status & 0x10)) request_stat_interrupt = true;
            if (next_mode == MODE_OAM && (this->lcd_status & 0x20)) request_stat_interrupt = true;

            if (request_stat_interrupt) {
                uint8_t iff = memory.bus_read(0xFF0F);
                memory.bus_write(0xFF0F, iff | 0x02);
            }
        }

        // Handle end of scanline
        if (this->dot_clock == 456) {
            if (this->window_visible_this_line) {
                this->window_lc++;
                this->window_visible_this_line = false;
            }

            this->dot_clock = 0;
            this->pushed_pixels = 0;
            this->ly++;
            
            if (ly == 144) {
                uint8_t iff = memory.bus_read(0xFF0F);
                memory.bus_write(0xFF0F, iff | 0x01); // VBlank Interrupt
                g_state.frame_ready.store(true);
            }

            if (ly > 153) {
                ly = 0;
                this->window_lc = 0;
            }

            // Evaluate LY == LYC
            if (this->ly == this->lyc) {
                this->lcd_status |= PPU_STAT_BIT_LYC_EQ_LY;

                if (this->lcd_status & 0x40) {
                    uint8_t iff = memory.bus_read(0xFF0F);
                    memory.bus_write(0xFF0F, iff | 0x02);
                }
            } else {
                this->lcd_status &= ~PPU_STAT_BIT_LYC_EQ_LY;
            }
        }
    }
}


void ppu_t::oam_scan() {
    this->sprite_count = 0;

    int obj_height = (this->lcd_control & PPU_LCDC_BIT_OBJ_SZ) ? 16 : 8;

    for (int i = 0; i < 40; i++) {
        int sprite_y = (int)this->oam[i * 4] - 16;

        if (ly < sprite_y || ly >= sprite_y + obj_height)
            continue;

        this->sprite_indices[this->sprite_count] = i;
        this->sprite_count ++;

        if (this->sprite_count == 10)
            break;
    }
}

void ppu_t::draw_bg() {
    uint8_t y_pos = this->scy + this->ly;
    uint16_t tile_row = (y_pos / 8) * 32;
    
    uint16_t tile_map_base = (this->lcd_control & PPU_LCDC_BIT_BG_TILE) ? 0x9C00 : 0x9800;
    bool unsigned_tile_addressing = this->lcd_control & (1 << 4);
    uint16_t tile_data_base = unsigned_tile_addressing ? 0x8000 : 0x9000;
    uint8_t line_within_tile = y_pos % 8;

    uint16_t bg_x = (uint16_t)this->scx + this->pushed_pixels;
    uint16_t tile_column = (bg_x / 8) & 31;

    uint16_t tile_address = tile_map_base + tile_row + tile_column;
    uint8_t raw_tile_id = memory.bus_read(tile_address);
    int tile_id = unsigned_tile_addressing ? raw_tile_id : (int8_t)raw_tile_id;

    uint16_t tile_location = tile_data_base + (tile_id * 16);

    uint8_t byte1 = memory.bus_read(tile_location + (line_within_tile * 2));
    uint8_t byte2 = memory.bus_read(tile_location + (line_within_tile * 2) + 1);

    int color_bit = 7 - (bg_x & 7);
    uint8_t color_lsb = (byte1 >> color_bit) & 1;
    uint8_t color_msb = (byte2 >> color_bit) & 1;
    uint8_t color_id = (color_msb << 1) | color_lsb;

    this->bg_color_id = color_id;
    uint8_t final_shade = (this->bgp >> (color_id * 2)) & 0x03;
    g_state.screen[ly * 160 + this->pushed_pixels] = colors[final_shade];
}

void ppu_t::draw_win(){
    if (this->ly < this->wy) return; // Window not visible on this line

    int actual_wx = ((int)this->wx) -7;
    int start_pixel = actual_wx < 0 ? 0 : actual_wx;
    
    if (start_pixel > this->pushed_pixels) return; // Not visible on this pixel

    uint16_t tile_row = (this->window_lc / 8) * 32;
    uint16_t tile_map_base = (this->lcd_control & PPU_LCDC_BIT_WIN_TILE) ? 0x9C00 : 0x9800;
    bool unsigned_tile_addressing = this->lcd_control & (1 << 4);
    uint16_t tile_data_base = unsigned_tile_addressing ? 0x8000 : 0x9000;
    uint8_t line_within_tile = this->window_lc % 8;

    int window_x = this->pushed_pixels - actual_wx;

    uint16_t tile_column = window_x / 8;
    uint16_t tile_address = tile_map_base + tile_row + tile_column;
    uint8_t raw_tile_id = memory.bus_read(tile_address);
    int tile_id = unsigned_tile_addressing ? raw_tile_id : (int8_t)raw_tile_id;

    uint16_t tile_location = tile_data_base + (tile_id * 16);

    uint8_t byte1 = memory.bus_read(tile_location + (line_within_tile * 2));
    uint8_t byte2 = memory.bus_read(tile_location + (line_within_tile * 2) + 1);

    int color_bit = 7 - (window_x % 8);
    uint8_t color_lsb = (byte1 >> color_bit) & 1;
    uint8_t color_msb = (byte2 >> color_bit) & 1;
    uint8_t color_id = (color_msb << 1) | color_lsb;

    this->bg_color_id = color_id;

    uint8_t final_shade = (this->bgp >> (color_id * 2)) & 0x03;
    g_state.screen[this->ly * 160 + this->pushed_pixels] = colors[final_shade];

    this->window_visible_this_line = true;
}

void ppu_t::draw_sprite(){
    uint8_t obj_height = (this->lcd_control & PPU_LCDC_BIT_OBJ_SZ) ? 16 : 8;

    int best_sprite = -1;
    int best_x = 999;
    int best_oam_idx = 999;
    uint8_t best_color = 0;
    uint8_t best_pal = 0;
    bool best_prio = false;

    for (int s = 0; s < sprite_count; s++) {
        int oam_idx = sprite_indices[s];
        int sprite_x = this->oam[oam_idx * 4 + 1] - 8;

        if (this->pushed_pixels >= sprite_x && this->pushed_pixels < sprite_x + 8) {

            int sprite_y = (int)this->oam[(oam_idx * 4)] - 16;
            uint8_t tile_id = this->oam[(oam_idx * 4) + 2];
            uint8_t attributes = this->oam[(oam_idx * 4) + 3];

            int line = ly - sprite_y;
            bool y_flip = attributes & (1 << 6);
            if (y_flip) line = obj_height - 1 - line;

            if (obj_height == 16) {
                tile_id &= 0xFE;

                if (line >= 8) {
                    tile_id++;
                    line -= 8;
                }
            }

            uint16_t tile_address = 0x8000 + (tile_id * 16) + (line * 2);
            uint8_t byte1 = memory.bus_read(tile_address);
            uint8_t byte2 = memory.bus_read(tile_address + 1);

            int pixel_in_sprite = this->pushed_pixels - sprite_x;
            bool x_flip = attributes & (1 << 5);
            int color_bit = x_flip ? pixel_in_sprite : 7 - pixel_in_sprite;

            uint8_t color_lsb = (byte1 >> color_bit) & 1;
            uint8_t color_msb = (byte2 >> color_bit) & 1;
            uint8_t color_id = (color_msb << 1) | color_lsb;

            if (color_id == 0) continue;

            bool is_better = false;
            if (best_sprite == -1) {
                is_better = true;
            }
            else {
                if (sprite_x < best_x) is_better = true;
                else if (sprite_x == best_x && oam_idx < best_oam_idx) is_better = true;
            }

            if (is_better) {
                best_sprite = s;
                best_x = sprite_x;
                best_oam_idx = oam_idx;
                best_color = color_id;

                best_pal = memory.bus_read((attributes & (1 << 4)) ? 0xFF49 : 0xFF48);
                best_prio = attributes & (1 << 7);
            }
        }
    }

    if (best_sprite != -1) {
        if (best_prio && this->bg_color_id != 0) {
            return;
        }
        uint8_t final_shade = (best_pal >> (best_color * 2)) & 0x03;
        g_state.screen[this->ly * 160 + this->pushed_pixels] = colors[final_shade];
    }
}

void ppu_t::tick_pixel_fifo() {
    if (this->lcd_control & PPU_LCDC_BIT_BG_EN) {
        // BG Enabled
        this->draw_bg();

        if (this->lcd_control & PPU_LCDC_BIT_WIN_EN) {
            // WIN Enabled
            this->draw_win();
        }
    } else {
        // BG Disabled, clear it to white
        this->bg_color_id = 0;
        g_state.screen[ly * 160 + this->pushed_pixels] = colors[0];
    }

    // Check the OBJ Enable Bit
    if (this->lcd_control & PPU_LCDC_BIT_OBJ_EN) {
        this->draw_sprite();
    }

    this->pushed_pixels++;
}

uint8_t ppu_mem_read(uint16_t address) {
	switch (address) {
		case PPU_REG_LCDC:
            return ppu.lcd_control;
        case PPU_REG_STAT:
            return ppu.lcd_status;
        case PPU_REG_SCY:
            return ppu.scy;
        case PPU_REG_SCX:
            return ppu.scx;
        case PPU_REG_LY:
            return ppu.ly;
        case PPU_REG_LYC:
            return ppu.lyc;
        case PPU_REG_BGP:
            return ppu.bgp;
        case PPU_REG_WY:
            return ppu.wy;
        case PPU_REG_WX:
            return ppu.wx;
        case PPU_REG_OBP0:
            return ppu.obp0;
        case PPU_REG_OBP1:
            return ppu.obp1;
	}

	return 0xFF;
}

void ppu_mem_write(uint16_t address, uint8_t value) {
	switch (address) {
		case PPU_REG_LCDC:
            ppu.lcd_control = value;
            break;
        case PPU_REG_STAT:
            ppu.lcd_status = value & (~0b11); // Bottom 2 bits RO
            break;
        case PPU_REG_SCY:
            ppu.scy = value;
            break;
        case PPU_REG_SCX:
            ppu.scx = value;
            break;
        case PPU_REG_LY:
            break; // RO
        case PPU_REG_LYC:
            ppu.lyc = value;
            break;
        case PPU_REG_BGP:
            ppu.bgp = value;
            break;
        case PPU_REG_WY:
            ppu.wy = value;
            break;
        case PPU_REG_WX:
            ppu.wx = value;
            break;
        case PPU_REG_OBP0:
            ppu.obp0 = value;
            break;
        case PPU_REG_OBP1:
            ppu.obp1 = value;
            break;
        case PPU_REG_DMA: {
            uint16_t source_address = value << 8;
            for (int i = 0; i < 160; i++) {
                ppu.oam[i] = memory.bus_read(source_address + i);
            }
            break;
        }
	}
}

uint8_t ppu_oam_read(uint16_t address) {
	return ppu.oam[address - PPU_OAM_MEM_START];
}

void ppu_oam_write(uint16_t address, uint8_t value) {
	ppu.oam[address - PPU_OAM_MEM_START] = value;
}

uint8_t ppu_vram_read(uint16_t address) {
	return ppu.vram[address - PPU_VRAM_MEM_START];
}

void ppu_vram_write(uint16_t address, uint8_t value) {
	ppu.vram[address - PPU_VRAM_MEM_START] = value;
}

REGISTER_MEMORY_REGION(ppu_mem_read, ppu_mem_write, PPU_REG_LCDC, PPU_REG_WX);
REGISTER_MEMORY_REGION(ppu_oam_read, ppu_oam_write, PPU_OAM_MEM_START, PPU_OAM_MEM_END);
REGISTER_MEMORY_REGION(ppu_vram_read, ppu_vram_write, PPU_VRAM_MEM_START, PPU_VRAM_MEM_END);