#include <gfx/drivers/gl_common.h>
#include <gfx/nanovg/nanovg.h>

#define FONT_ALIAS "default"
#define FONT_SIZE 24

#define VKEY_STATE_PRESSED  1
#define VKEY_STATE_SELECTED 2
#define KEY_RADIUS 4

#define BACKGROUND_R 20
#define BACKGROUND_G 20
#define BACKGROUND_B 20
#define BACKGROUND_A 255

#define KEY_BACKGROUND_R 10
#define KEY_BACKGROUND_G 10
#define KEY_BACKGROUND_B 10
#define KEY_BACKGROUND_A 255

#define KEY_TEXT_R 250
#define KEY_TEXT_G 250
#define KEY_TEXT_B 250
#define KEY_TEXT_A 255

struct vkey {
	int key_code;
	int modifier;
};

struct vkey_event {
	struct vkey *vkey;
	int down;
};

struct vkey_button {
	char *label;
	struct vkey vkey;
	int size;
	int state; //
};

struct vkey_layout {
	struct vkey_button **keys[5];
	int rows;
};

struct vkeyboard {
	struct vkey_layout *layout[3];
	int active_layout;
};

struct vkeyboard *vkeyboard;

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
	x += 2;
	y += 2;
	width -=4;
	height -=4;

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

	struct vkey_layout *layout = vkeyboard->layout[vkeyboard->active_layout];

	int row_height = height / layout->rows;
	int py = y;
	for(int row = 0; row < layout->rows; row++) {
		struct vkey_button **keys = layout->keys[row];
		int cols = get_cols(keys);
		int key_size = width / cols;

		int px = x;
		for(int col=0; col<cols; col++) {
			struct vkey_button *button = keys[col];
			int key_width = key_size * button->size;
			if (col+1 == cols) {
				key_width = width - px;
			}
			draw_button(vg, button, px, py, key_width, row_height);
			px += key_width;
		}
		py += row_height;
	}
}

void vkey_init(struct NVGcontext* vg, char *font_path) {
	static struct vkey_button a = {"a", {97, 0}, 1, 0};
	static struct vkey_button b = {"b", {98, 0}, 1, 0};
	static struct vkey_button c = {"c", {99, 0}, 1, 0};
	static struct vkey_button d = {"d", {100, 0}, 1, 0};
	static struct vkey_button e = {"e", {101, 0}, 1, 0};

	static struct vkey_button *row1[] = {
			&a, &b, &c, NULL
	};

	static struct vkey_button *row2[] = {
			&d, &e, NULL
	};

	static struct vkey_layout layout_1 = {
			{row1, row2}, 2
	};
	static struct vkeyboard keyb_pc = {
			{&layout_1}, 0
	};

	vkeyboard = &keyb_pc;

	nvgCreateFont(vg, FONT_ALIAS, font_path);
}






