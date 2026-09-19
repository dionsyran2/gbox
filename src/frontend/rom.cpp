#include <frontend.h>
#include <common.h>
#include <GLFW/glfw3.h>
#include <nfd.h>
#include <stdio.h>
#include <cartridge.h>

namespace frontend {
    void open_rom() {
        nfdu8char_t* out_path;
        nfdu8filteritem_t filters[2] = { {"Gameboy Roms", "gb" }, {"Gameboy Color Roms", "gbc" } };
        nfdopendialogu8args_t args = { 0 };
        args.filterList = filters;
        args.filterCount = 2;

        nfdresult_t result = NFD_OpenDialogU8_With(&out_path, &args);
        if (result == NFD_OKAY) {
            cartridge.load_rom(out_path);
            NFD_FreePathU8(out_path);
        }
        else if (result == NFD_CANCEL) {
            //printf("User Cancelled\n");
        }
        else {
            printf("NFD Error: %s\n", NFD_GetError());
        }
    }
}