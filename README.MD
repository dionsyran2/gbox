# What is GBox?
### GBox Is a DMG GameBoy Emulator... In simpler words, the OG Monocrome GameBoy.

# Pre-Compiled Binaries
### There are pre-compiled binaries in the releases section

# Compiling

To compile gbox, you will need 2 things:
A compiler. The Makefile uses mingw on windows and g++ on linux.
You can change the compiler by setting the CXX Enviroment Variable!

The GLFW library.
Example Commands:
    linux: ```sudo apt install libglfw3-dev```
    windows: ```pacman -S mingw-w64-x86_64-glfw```
 
#### After all of these, you can just run ```make``` or ```make all``` to compile!
# Supported Features
Full CPU/PPU/APU Emulation. (Fancy words to say that it can display an image and generate audio)
SRAM Save/Load Support. (You can save the SRAM, the battery backed RAM that would be on a physical cartridge to store save data)
Well... Thats it, what else do you want an emulator to do?
Ok Ok... I will add an emulation speed setting and stuff you win...