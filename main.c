#include "Resource.h"

#define PPU_BG1_SCROLL_X ((volatile unsigned*)0x2810)
#define PPU_BG1_SCROLL_Y ((volatile unsigned*)0x2811)
#define PPU_BG1_ATTR ((volatile unsigned*)0x2812)
#define PPU_BG1_CTRL ((volatile unsigned*)0x2813)
#define PPU_BG1_TILE_ADDR ((volatile unsigned*)0x2814)

#define PPU_BG2_CTRL ((volatile unsigned*)0x2819)

#define PPU_BG1_SEGMENT_ADDR ((volatile unsigned*)0x2820)

#define PPU_SPRITE_CTRL ((volatile unsigned*)0x2842)

#define PPU_INT_CTRL ((volatile unsigned*)0x2862)
#define PPU_INT_CLEAR ((volatile unsigned*)0x2863)

#define PPU_COLOR ((volatile unsigned*)0x2B00)

#define SPU_CHANNEL_ENABLE ((volatile unsigned*)0x3400)
#define SPU_MAIN_VOLUME ((volatile unsigned*)0x3401)
#define SPU_ENV_RAMP_DOWN ((volatile unsigned*)0x340A)
#define SPU_CHANNEL_STOP ((volatile unsigned*)0x340B)
#define SPU_CONTROL ((volatile unsigned*)0x340D)
#define SPU_CHANNEL_STATUS ((volatile unsigned*)0x340F)
#define SPU_WAVE_OUT_L ((volatile unsigned*)0x3412)
#define SPU_WAVE_OUT_R ((volatile unsigned*)0x3413)
#define SPU_CHANNEL_ENV_MODE ((volatile unsigned*)0x3415)

#define SPU_CH0_WAVE_ADDR ((volatile unsigned*)0x3000)
#define SPU_CH0_MODE ((volatile unsigned*)0x3001)
#define SPU_CH0_PAN ((volatile unsigned*)0x3003)
#define SPU_CH0_ENVELOPE0 ((volatile unsigned*)0x3004)
#define SPU_CH0_ENVELOPE_DATA ((volatile unsigned*)0x3005)
#define SPU_CH0_ENVELOPE1 ((volatile unsigned*)0x3006)
#define SPU_CH0_PREV_WAVE_DATA ((volatile unsigned*)0x3009)
#define SPU_CH0_ENVELOPE_LOOP_CTRL ((volatile unsigned*)0x300A)
#define SPU_CH0_WAVE_DATA ((volatile unsigned*)0x300B)

#define SPU_CH0_PHASE_HI ((volatile unsigned*)0x3200)
#define SPU_CH0_PHASE_ACC_HI ((volatile unsigned*)0x3201)
#define SPU_CH0_PHASE_LO ((volatile unsigned*)0x3204)
#define SPU_CH0_PHASE_ACC_LO ((volatile unsigned*)0x3205)

#define GPIO_C_DATA ((volatile unsigned*)0x3D0B)
#define GPIO_C_BUFFER ((volatile unsigned*)0x3D0C)
#define GPIO_C_DIR ((volatile unsigned*)0x3D0D)
#define GPIO_C_ATTRIB ((volatile unsigned*)0x3D0E)
#define GPIO_C_MASK ((volatile unsigned*)0x3D0F)

#define SYSTEM_CTRL ((volatile unsigned*)0x3D20)
#define INT_CTRL ((volatile unsigned*)0x3D21)
#define INT_CLEAR ((volatile unsigned*)0x3D22)
#define WATCHDOG_CLEAR ((volatile unsigned*)0x3D24)

static volatile int vblank_flag = 0;

static const char *hextable = "0123456789abcdef";

static int tilemap[2048];

#define make_color(r, g, b) ((r << 10) | (g << 5) | (b << 0))

void print_dec(int y, int x, unsigned int value){
	int digits[5];
	int i, j, len = 0;

	do {
		digits[len++] = value % 10;
	} while(value /= 10);

	for (i = 4; i >= len; i--) {
		tilemap[64*y + x + i] = ' ';
	}
	for (i = len - 1, j = 0; i >= 0; i--, j++) {
		tilemap[64*y + x + i] = hextable[digits[j]];
	}
}

void print_hex_addr(int y, int x, unsigned long value){
	int i;
	for(i = 5; i >= 0; i--, value >>= 4){
		tilemap[64*y + x + i] = hextable[value & 0x0f];
	}
}

void print_hex(int y, int x, unsigned int value){
	int i;
	for(i = 3; i >= 0; i--, value >>= 4){
		tilemap[64*y + x + i] = hextable[value & 0x0f];
	}
}

void print_hex2(int y, int x, unsigned int value){
	int i;
	for(i = 1; i >= 0; i--, value >>= 4){
		tilemap[64*y + x + i] = hextable[value & 0x0f];
	}
}

void print_string(int y, int x, const char *str){
	while(*str){
		tilemap[64*y + (x++)] = *str++;
	}
}

void clear_tilemap(){
	int i;
	for (i = 0; i < sizeof(tilemap); i++)
		tilemap[i] = ' ';
}

void wait_vblank(){
	vblank_flag = 1;
	while(vblank_flag) {};
}

void IRQ0(void) __attribute__ ((ISR));

void IRQ0(void){
	*PPU_INT_CLEAR = 1;
	if(vblank_flag) {
		vblank_flag = 0;
	}
}

int main(){
	int i;

	*SYSTEM_CTRL = 0;
	*INT_CTRL = 0;
	*WATCHDOG_CLEAR = 0x55aa;

	// Configure PPU
	*PPU_BG1_SCROLL_X = 0;
	*PPU_BG1_SCROLL_Y = 0;
	*PPU_BG1_ATTR = 0;
	*PPU_BG1_CTRL = 0x0a;
	*PPU_BG2_CTRL = 0;
	*PPU_SPRITE_CTRL = 0;
	*PPU_INT_CTRL = 1;

	*PPU_BG1_TILE_ADDR = (int) tilemap;
	*PPU_BG1_SEGMENT_ADDR = RES_FONT_BIN_SA >> 6;

	PPU_COLOR[0] = make_color(29, 26, 15);
	PPU_COLOR[1] = make_color(0, 8, 16);

	clear_tilemap();
	print_string(2, 2, "Sound test");

	// Enable interrupts
	__asm__("irq on");

	// Configure GPIO (turn on V.Smile audio output)
	*GPIO_C_DATA &= ~0x60;
	*GPIO_C_DIR |= 0x60;
	*GPIO_C_ATTRIB |= 0x60;
	*GPIO_C_MASK = 0;

	// Wait 100 frames until starting demo
	for(i = 0; i < 100; i++) {
		wait_vblank();
	}

	// Initialize SPU
	*SPU_CONTROL = 0x01c8; // initialize SPU, highest volume level, interpolation off
	*SPU_MAIN_VOLUME = 0x7f; // full volume

	// Set phase of sample
	// Phase = Sample rate * (2**19 / 281250)
	*SPU_CH0_PHASE_HI = 0x1;
	*SPU_CH0_PHASE_LO = 0x4120; // 44100 Hz
	*SPU_CH0_PHASE_ACC_HI = 0;
	*SPU_CH0_PHASE_ACC_LO = 0;

	// Set other channel settings
	*SPU_CH0_WAVE_ADDR = RES_SALCON_U16_SA;
	*SPU_CH0_MODE = 0x5000 | (RES_SALCON_U16_SA >> 16UL); // (unsigned) 16-bit PCM, auto-end
	*SPU_CH0_PAN = (0x40 << 8) | 0x7f; // Pan to center, full volume
	*SPU_CH0_ENVELOPE_DATA = 0x7f; // Set envelope volume to full
	*SPU_CH0_ENVELOPE_LOOP_CTRL = 0;
	*SPU_CH0_PREV_WAVE_DATA = 0x8000; // Reset channel wave data to zero point
	*SPU_CH0_WAVE_DATA = 0x8000;

	// Enable channel
	*SPU_ENV_RAMP_DOWN &= ~1;
	*SPU_CHANNEL_ENV_MODE |= 1;
	*SPU_CHANNEL_STOP |= 1;
	*SPU_CHANNEL_ENABLE |= 1;

	// Print some interesting data
	for(;;){
		wait_vblank();

		print_string(5, 2, "Out L:");
		print_string(6, 2, "Out R:");

		print_hex(5, 11, *SPU_WAVE_OUT_L);
		print_hex(6, 11, *SPU_WAVE_OUT_R);

		print_string(9, 2, "Enable:");
		print_string(10, 2, "Status:");
		print_string(11, 2, "Stop:");

		print_string(9, 11, (*SPU_CHANNEL_ENABLE & 1) ? "On " : "Off");
		print_string(10, 11, (*SPU_CHANNEL_STATUS & 1) ? "On " : "Off");
		print_string(11, 11, (*SPU_CHANNEL_STOP & 1) ? "On " : "Off");

		print_string(13, 2, "Wave address:");
		print_string(15, 2, "Prev wave data:");
		print_string(16, 2, "Wave data:");
		print_string(18, 2, "Phase:");
		print_string(19, 2, "Phase acc:");
		print_string(21, 2, "EDD: ");

		print_hex_addr(13, 18, ((long)(*SPU_CH0_MODE & 0x3f) << 16) | (long)(*SPU_CH0_WAVE_ADDR));
		print_hex(15, 18, *SPU_CH0_PREV_WAVE_DATA);
		print_hex(16, 18, *SPU_CH0_WAVE_DATA);
		print_hex_addr(18, 18, ((long)(*SPU_CH0_PHASE_HI) << 16) | (long)(*SPU_CH0_PHASE_LO));
		print_hex_addr(19, 18, ((long)(*SPU_CH0_PHASE_ACC_HI) << 16) | (long)(*SPU_CH0_PHASE_ACC_LO));
		print_hex2(21, 18, *SPU_CH0_ENVELOPE_DATA & 0x7f);

		if (*SPU_CONTROL & 0x20) {
			print_string(24, 2, "FIFO overflow flag set");
		}
	}

	return 0;
}
