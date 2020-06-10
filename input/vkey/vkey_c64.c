#include <stdio.h>
#include <stdlib.h>
#include "vkey.h"
#include "vkey_c64.h"

static char *key_labels_row1[] = {"<-", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "+", "-", "$", "HOME", "DEL", NULL};
static char *key_labels_row2[] = {"CTRL", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "@", "*", "|", "RESTORE", NULL};
static char *key_labels_row3[] = {"STOP", "LOCK", "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", ":", "=", "RETURN", NULL};
static char *key_labels_row4[] = {"C=", "SHIFT", "Z", "X", "C", "V", "B", "N", "M", ".", ",", "/", "SHIFT", "UP/DN", "LT/RT", NULL};
static char *key_labels_row5[] = {"F1", "F2", "SPACE", "F3", "F4", NULL};

static char **key_rows[] = {key_labels_row1, key_labels_row2, key_labels_row3, key_labels_row4,key_labels_row5, NULL};

static struct vkey_layout layout_1;
static struct vkeyboard keyb_c64 = {
		{&layout_1}, 0
};

static void set_key_size(struct vkey_layout *layout, char *label, int size) {
	for(int row=0; row<layout->rows; row++) {
		int col = 0;
		struct vkey_button *button;
		while ((button = layout_1.keys[row][col++]) != NULL) {
			if (!strcmp(button->label, label)) {
				button->size = size;
			}
			col++;
		}
	}
}

struct vkeyboard *vkey_init_c64() {
	int rows = 0;
	while(key_rows[rows]) rows++;

	RARCH_LOG("vkey c64 rows %d", rows);

	for(int i=0; i<rows; i++) {
		RARCH_LOG("vkey c64 create row %d", i);
		layout_1.keys[i] = keyb_create_row(key_rows[i]);
	}
	layout_1.rows = rows;

	set_key_size(&layout_1, "CTRL", 2);
	set_key_size(&layout_1, "RESTORE", 2);
	set_key_size(&layout_1, "RETURN", 2);
	set_key_size(&layout_1, "SHIFT", 2);
	set_key_size(&layout_1, "SPACE", 4);

	return &keyb_c64;
}
