#include <rp6502.h>
#include <fcntl.h>

static void load_rom_to_xram(const char *name, unsigned xaddr, unsigned total) {
    int fd = open(name, O_RDONLY);
    if (fd < 0) return;
    const unsigned chunk = 16384;
    while (total > 0) {
        unsigned n = total < chunk ? total : chunk;
        read_xram(xaddr, n, fd);
        xaddr += n;
        total -= n;
    }
    close(fd);
}

void show_splash_screen(void) {
    load_rom_to_xram("ROM:title_screen.bin",     0x0000, 57600);
    load_rom_to_xram("ROM:title_screen_pal.bin", 0xF000,   512);
}