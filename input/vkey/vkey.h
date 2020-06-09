#ifndef __VKEY_H
#define __VKEY_H

void vkey_render(struct NVGcontext* vg, int x, int y, int width, int height);
void vkey_init(struct NVGcontext* vg, char *font_path);

#endif
