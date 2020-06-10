#include "vkey.h"

#define FONT_ALIAS "default"
#define FONT_SIZE 18

#define KEY_PADDING       2
#define KEYBOARD_PADDING  8

#define VKEY_STATE_PRESSED  1
#define VKEY_STATE_SELECTED 2
#define KEY_RADIUS 4

#define BACKGROUND_R 0x10
#define BACKGROUND_G 0x0E
#define BACKGROUND_B 0x0F
#define BACKGROUND_A 255

#define KEY_BACKGROUND_R 0x0A
#define KEY_BACKGROUND_G 0x08
#define KEY_BACKGROUND_B 0x09
#define KEY_BACKGROUND_A 255

#define KEY_TEXT_R 0x7E
#define KEY_TEXT_G 0x78
#define KEY_TEXT_B 0x78
#define KEY_TEXT_A 255

static struct vkeyboard *vkeyboard;

static int get_cols(struct vkey_button *keys[]) {
	int col = 0;
	while (keys[col]) col++;
	return col;
}

static int get_row_size(struct vkey_button *keys[]) {
	int size = 0;
	int col = 0;
	while (keys[col]) size += keys[col++]->size;
	return size;
}

static void draw_button(struct NVGcontext* vg, struct vkey_button *button, int x, int y, int width, int height) {

	// emulate margins
	x += KEY_PADDING;
	y += KEY_PADDING;
	width  -= KEY_PADDING * 2;
	height -= KEY_PADDING * 2 ;

	// draw key background
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, width, height, KEY_RADIUS);
	nvgFillColor(vg, nvgRGBA(KEY_BACKGROUND_R, KEY_BACKGROUND_G, KEY_BACKGROUND_B, KEY_BACKGROUND_A));
	nvgFill(vg);

	// measure text size
	float bounds[4];
	nvgTextBounds(vg, 0, 0, button->label, NULL, bounds);

	int text_width  = bounds[2] - bounds[0];
	int text_height = bounds[3] - bounds[1];
	int text_ascent = -bounds[1];

	// center text
	int text_x = (width - text_width) / 2;
	int text_y = (height - text_height) / 2 + text_ascent;

	// draw text
	nvgFillColor(vg, nvgRGBA(KEY_TEXT_R, KEY_TEXT_G, KEY_TEXT_B, KEY_TEXT_A));
	nvgText(vg, x + text_x, y + text_y, button->label, NULL);

}

void vkey_render(struct NVGcontext* vg, int x, int y, int width, int height) {
	nvgFontSize(vg, FONT_SIZE);
	nvgFontFace(vg, FONT_ALIAS);

	// draw keyboard background
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, width, height, 0);
	nvgFillColor(vg, nvgRGBA(BACKGROUND_R, BACKGROUND_G, BACKGROUND_B, BACKGROUND_A));
	nvgFill(vg);

	// use padding
	x += KEYBOARD_PADDING;
	y += KEYBOARD_PADDING;
	width  -= KEYBOARD_PADDING * 2;
	height -= KEYBOARD_PADDING * 2 ;

	struct vkey_layout *layout = vkeyboard->layout[vkeyboard->active_layout];

	int row_height = height / layout->rows;
	int py = y;
	for(int row = 0; row < layout->rows; row++) {
		struct vkey_button **keys = layout->keys[row];
		int cols = get_cols(keys);
		int row_size = get_row_size(keys);
		int key_size = width / row_size;

		int px = x;
		for(int col=0; col<cols; col++) {
			struct vkey_button *button = keys[col];
			int key_width = key_size * button->size;
			if (col+1 == cols) {
				key_width = width + KEYBOARD_PADDING - px;
			}
			draw_button(vg, button, px, py, key_width, row_height);
			px += key_width;
		}
		py += row_height;
	}
}

void vkey_init(struct NVGcontext* vg, char *font_path, struct vkeyboard *keyboard) {
	vkeyboard = keyboard;
	nvgCreateFont(vg, FONT_ALIAS, font_path);
}

struct vkey_button **keyb_create_row(char *labels[]) {
	int count = 0;
	while(labels[count]) count++;

	struct vkey_button **row = calloc(count+1, sizeof(struct vkey_button*));
	for(int i=0; i<count; i++) {
		struct vkey_button *button = calloc(1, sizeof(struct vkey_button));
		row[i] = button;
		button->label = labels[i];
		RARCH_LOG("keyb create key %s", button->label);
		button->size = 1;
	}
	row[count] = NULL;
	RARCH_LOG("keyb row created with %d keys", count);
	return row;
}
