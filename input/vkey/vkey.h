#ifndef __VKEY_H
#define __VKEY_H

#include <gfx/drivers/gl_common.h>
#include <gfx/nanovg/nanovg.h>

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
	struct vkey_button **keys[10];
	int rows;
};

struct vkeyboard {
	struct vkey_layout *layout[3];
	int active_layout;
};

void vkey_render(struct NVGcontext* vg, int x, int y, int width, int height);
void vkey_init(struct NVGcontext* vg, char *font_path, struct vkeyboard *keyboard);
struct vkey_button **keyb_create_row(char *labels[]);

#endif
