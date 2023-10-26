#include <assert.h>
#include <cairo.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftmodapi.h>
#include <cairo-ft.h>

#include "scfg.h"
#include "util.h"
#include "game.h"
#include "fuyunix.h"
#include <dlfcn.h>

struct {
	char *name;
	struct wl_cursor *cursor;
} cursors[CURSOR_COUNT] = {
	[CURSOR_ARROW] = {"left_ptr", NULL},
};

struct Buffer {
	struct wl_buffer *wl_buf;
	int width;
	int height;
	int stride;
	int fd;
	uint8_t *data;
	size_t data_sz;
	cairo_t *cr;
	cairo_surface_t *surf;
};

struct Wayland {
	struct wl_display *display;
	struct wl_shm *shm;
	struct wl_compositor *compositor;
	struct wl_surface *surface;
	struct xdg_wm_base *xdg_wm_base;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	struct wl_seat *seat;
	struct wl_output *output;
	struct zxdg_decoration_manager_v1 *decor_manager;
	struct zxdg_toplevel_decoration_v1 *top_decor;

	struct xkb_state *xkb_state;
	struct xkb_keymap  *xkb_keymap;
	struct xkb_context *xkb_context;

	struct {
		int x;
		int y;
		struct wl_surface *surface;
		struct wl_cursor_theme *theme;
		struct wl_cursor *cursor;
		int curimg;
		struct wl_cursor_image *image;
		struct wl_pointer *pointer;
		uint32_t serial;
	} pointer;

	int width;
	int height;

	struct game_Input game_input;

	struct Buffer buffer;

	FT_Library ft_lib;
	FT_Face    ft_face;
	cairo_font_face_t *font_face;

	bool configured;
	bool redraw;
	bool quit;
};

struct Wayland wayland;

#define CAIRO_RGBA(c) (c.r / 255.0), (c.g / 255.0), (c.b / 255.0), (c.a / 255.0)

void
platform_Clip(struct game_Rect r)
{
	cairo_t *cr = wayland.buffer.cr;
	cairo_rectangle(cr, r.x, r.y, r.w, r.h);
	cairo_clip(cr);
}

void
platform_ResetClip(void)
{
	cairo_t *cr = wayland.buffer.cr;
	cairo_reset_clip(cr);
}

void
platform_DrawTrail(int x1, int y1, int x2, int y2, int size, struct game_Color color)
{
	cairo_t *cr = wayland.buffer.cr;
	cairo_pattern_t *pat;
	pat = cairo_pattern_create_radial(x1, y1, size/2, x2, y2, size/2);

	color.a = 0;
	cairo_pattern_add_color_stop_rgba(pat, 1, CAIRO_RGBA(color));

	color.a = 0xff;
	cairo_pattern_add_color_stop_rgba(pat, 0, CAIRO_RGBA(color));

	cairo_move_to(cr, x1, y1);
	cairo_line_to(cr, x1, y2);
	cairo_line_to(cr, x2, y2);
	cairo_line_to(cr, x2, y1);
	cairo_close_path(cr);

	cairo_set_source(cr, pat);
	cairo_fill(cr);
	cairo_pattern_destroy(pat);
}

game_Texture *
platform_LoadTexture(char *file)
{
	cairo_surface_t *surf =  cairo_image_surface_create_from_png(file);
	cairo_status_t s = cairo_surface_status(surf);
	if (s != CAIRO_STATUS_SUCCESS) {
		return NULL;
	}
	return (game_Texture*)surf;
}

void
platform_DestroyTexture(game_Texture *t)
{
	cairo_surface_destroy((cairo_surface_t*)t);
}

void
platform_Log(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

bool
platform_ReadSaveData(struct game_Data *data)
{
	int level = readSaveFile();
	data->level = level;
	return true;
}

void
platform_WriteSaveData(struct game_Data *data)
{
	writeSaveFile(data->level);
}

void
platform_RenderText(char *text, int size, struct game_Color fg, int x, int y)
{
	cairo_t *cr = wayland.buffer.cr;
	cairo_save(cr);
	cairo_set_font_size(cr, (double)size);
	cairo_move_to(cr, x, y + size/2);
	cairo_set_source_rgba(cr, CAIRO_RGBA(fg));
	cairo_show_text(cr, text);
	cairo_restore(cr);
}

void
platform_MeasureText(char *text, int size, int *w, int *h)
{
	cairo_text_extents_t ext;
	cairo_t *cr = wayland.buffer.cr;
	cairo_save(cr);
	cairo_set_font_size(cr, (double)size);
	cairo_text_extents(cr, text, &ext);
	cairo_restore(cr);

	if (w) *w = ext.width;
	if (h) *h = ext.height;
}

void
platform_FillRect(struct game_Color c, struct game_Rect *rect)
{
	cairo_t *cr = wayland.buffer.cr;
	cairo_set_source_rgba(cr, CAIRO_RGBA(c));
	if (rect == NULL) {
		cairo_rectangle(cr, 0, 0, wayland.buffer.width, wayland.buffer.height);
	} else {
		cairo_rectangle(cr, rect->x, rect->y, rect->w, rect->h);
	}
	cairo_fill(cr);
}

void
platform_DrawTexture(game_Texture *tex, struct game_Rect *src, struct game_Rect *dst)
{
	struct Wayland *wl = &wayland;
	cairo_t *cr = wl->buffer.cr;
	cairo_save(cr);

	cairo_surface_t *t = (cairo_surface_t *)tex;

	if (src != NULL) {
		t = cairo_surface_create_for_rectangle(t,
				src->x, src->y, src->w, src->h);
	}

	if (dst != NULL) {
		cairo_set_source_surface(cr, t, dst->x, dst->y);
	} else {
		// TODO: render to entire surface
		cairo_set_source_surface(cr, t, 0, 0);
	}
	cairo_paint(cr);
	if ((cairo_surface_t *)t != (cairo_surface_t *)tex) {
		cairo_surface_destroy(t);
	}

	cairo_restore(cr);
}

void
platform_Clear(struct game_Color c)
{
	cairo_t *cr = wayland.buffer.cr;
	cairo_set_source_rgba(cr, CAIRO_RGBA(c));
	cairo_paint(cr);
}

void
platform_DrawLine(struct game_Color c, int x1, int y1, int x2, int y2)
{
	cairo_t *cr = wayland.buffer.cr;
	cairo_set_source_rgba(cr, CAIRO_RGBA(c));
	cairo_move_to(cr, x1, y1);
	cairo_line_to(cr, x2, y2);
	cairo_stroke(cr);
}

struct Key {
	xkb_keysym_t key;
	KeySym sym;
	int player;
};
static struct {
	struct Key *key;
	int keylen;
	int is_key_allocated;
} keys;

static_assert(KEY_COUNT == 8, "Update keyList");

static char *keysList[] = {
	[KEY_UP]     = "up",
	[KEY_DOWN]   = "down",
	[KEY_LEFT]   = "left",
	[KEY_RIGHT]  = "right",
	[KEY_PAUSE]  = "pause",
	[KEY_SHOOT]  = "shoot",
	[KEY_QUIT]   = "quit",
};

static int
stringToKey(char *s)
{
	for (size_t i = 0; i < sizeof(keysList) / sizeof(keysList[0]); i++) {
		if (strcmp(s, keysList[i]) == 0)
			return i;
	}
	return -1;
}

static struct Key defaultkey[] = {
	{XKB_KEY_q,       KEY_QUIT,   0},
	{XKB_KEY_Escape,  KEY_PAUSE,  0},
	/* Player 1 */
	{XKB_KEY_h,        KEY_LEFT,   0},
	{XKB_KEY_j,        KEY_DOWN,   0},
	{XKB_KEY_k,        KEY_UP,     0},
	{XKB_KEY_l,        KEY_RIGHT,  0},
	{XKB_KEY_u,        KEY_SHOOT,  0},
	/* Player 2 */
	{XKB_KEY_a,        KEY_LEFT,   1},
	{XKB_KEY_s,        KEY_DOWN,   1},
	{XKB_KEY_w,        KEY_UP,     1},
	{XKB_KEY_d,        KEY_RIGHT,  1},
	{XKB_KEY_e,        KEY_SHOOT,  1},
};

// XXX: it is possible to share most of this code with the sdl version.
void
loadConfig(void)
{
	char filepath[PATH_MAX];
	struct Key *keylist = NULL;
	int keylist_len = 0;
	struct scfg_block block;

	if (!getConfigFile(filepath, sizeof(filepath))) {
		goto default_config;
	}
	if (scfg_load_file(&block, filepath) < 0) {
		perror(filepath);
		goto default_config;
	}

	for (size_t i = 0; i < block.directives_len; i++) {
		struct scfg_directive *d = &block.directives[i];
		if (strcmp(d->name, "keys") == 0) {
			if (d->params_len > 0) {
				fprintf(stderr, "%s:%d: Expected 0 params got %zu params\n",
						filepath, d->lineno, d->params_len);
				exit(1);
			}
			struct scfg_block *child = &d->children;

			for (size_t i = 0; i < child->directives_len; i++) {
				struct Key key;
				struct scfg_directive *d = &child->directives[i];
				if (d->params_len != 2) {
					fprintf(stderr, "%s:%d: Expected 2 params, got %zu\n",
							filepath, d->lineno, d->params_len);
					exit(1);
				};
				int sym = stringToKey(d->name);
				if (sym < 0) {
					fprintf(stderr, "%s:%d: Invalid directive %s\n", filepath,
							d->lineno, d->name);
					exit(1);
				}
				xkb_keysym_t keysym = xkb_keysym_from_name(d->params[0], 0);
				if (keysym == XKB_KEY_NoSymbol) {
					fprintf(stderr, "%s:%d: Invalid name %s\n",
							filepath, d->lineno, d->params[0]);
					exit(1);
				}
				int player = atoi(d->params[1]);
				if (player <= 0) {
					fprintf(stderr, "%s:%d: Invalid number %s\n",
							filepath, d->lineno, d->params[1]);
					exit(1);
				}

				key.sym = sym;
				key.key = keysym;
				key.player = player-1;

				keylist_len++;
				keylist = erealloc(keylist, keylist_len * sizeof(struct Key));
				keylist[keylist_len-1] = key;
			}
		}
	}

	scfg_block_finish(&block);

	if (keylist != NULL) {
		keys.key = keylist;
		keys.keylen = keylist_len;
		keys.is_key_allocated = 1;
	} else
default_config:
	{
		keys.key = defaultkey;
		keys.keylen = sizeof(defaultkey) / sizeof(defaultkey[0]);
		keys.is_key_allocated = 0;
	}
}

void
listFunc(void)
{
	for (size_t i = 0; i < sizeof(keysList) / sizeof(keysList[0]); i++)
		printf("%s\n", keysList[i]);
}

int
gameKey(xkb_keysym_t key, int *p)
{
	for (int i = 0; i < keys.keylen; i++) {
		if (keys.key[i].key == key) {
			*p = keys.key[i].player;
			return keys.key[i].sym;
		}
	}
	*p = 0;
	switch (key) {
	case XKB_KEY_Up:
		return KEY_UP;
	case XKB_KEY_Down:
		return KEY_DOWN;
	case XKB_KEY_Left:
		return KEY_LEFT;
	case XKB_KEY_Right:
		return KEY_RIGHT;
	case XKB_KEY_q:
		return KEY_QUIT;
	case XKB_KEY_Escape:
		return KEY_PAUSE;
	case XKB_KEY_Return: // fallthrough
	case XKB_KEY_space:
		return KEY_SELECT;
	}
	return -1;
}

void
keyboard_handle_keymap(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t format, int32_t fd, uint32_t size)
{
	struct Wayland *wl = data;

	if (wl->xkb_context == NULL)
		wl->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	char *s = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (s == MAP_FAILED) {
		fprintf(stderr, "failed to get keyboard keymap\n");
		close(fd);
		exit(1);
	}
	if (wl->xkb_keymap != NULL) {
		xkb_keymap_unref(wl->xkb_keymap);
		wl->xkb_keymap = NULL;
	}
	if (wl->xkb_state != NULL) {
		xkb_state_unref(wl->xkb_state);
		wl->xkb_state = NULL;
	}

	wl->xkb_keymap = xkb_keymap_new_from_string(wl->xkb_context, s,
			XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

	wl->xkb_state = xkb_state_new(wl->xkb_keymap);

	munmap(s, size);
	close(fd);
}

void
keyboard_handle_key(void *data, struct wl_keyboard *wl_keyboard, uint32_t
		serial, uint32_t time, uint32_t key, uint32_t keyState)
{
	struct Wayland *wl = data;

	if (wl->xkb_state == NULL)
		return;

	// the scancode from this event is the Linux evdev scancode. To convert
	// this to an XKB scancode, we must add 8 to the evdev scancode.
	key += 8;

	xkb_keysym_t keysym = xkb_state_key_get_one_sym(wl->xkb_state, key);
	if (keysym == XKB_KEY_NoSymbol)
		return;

	int player = 0;
	int k = gameKey(keysym, &player);
	if (k < 0)
		return;

	if (keyState == WL_KEYBOARD_KEY_STATE_RELEASED) {
		wl->game_input.keys[player][k] = KEY_RELEASED;
	} else {
		wl->game_input.keys[player][k] = KEY_PRESSED;
	}
}

static void
keyboard_handle_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
		int32_t rate, int32_t delay)
{
}

void
keyboard_handle_modifiers(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
		uint32_t mods_locked, uint32_t group)
{
	struct Wayland *wl = data;
	xkb_state_update_mask(wl->xkb_state, mods_depressed, mods_latched,
			mods_locked, 0, 0, group);
}

static void
keyboard_handle_enter(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
}
static void
keyboard_handle_leave(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t serial, struct wl_surface *surface)
{
}
struct wl_keyboard_listener keyboard_listener = {
	.enter = keyboard_handle_enter,
	.leave = keyboard_handle_leave,
	.keymap = keyboard_handle_keymap,
	.key = keyboard_handle_key,
	.modifiers = keyboard_handle_modifiers,
	.repeat_info = keyboard_handle_repeat_info,
};
static void
pointer_handle_enter(void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, struct wl_surface *surface,
		wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	struct Wayland *wl = data;
	wl->pointer.serial = serial;

	wl_pointer_set_cursor(wl_pointer, serial, wl->pointer.surface,
			wl->pointer.image->hotspot_x, wl->pointer.image->hotspot_y);
}

static void
pointer_handle_leave(void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, struct wl_surface *surface)
{
}
void
pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
	uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
}
void
pointer_handle_motion(void *data, struct wl_pointer *wl_pointer,
	uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
}
void
pointer_handle_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time,
		uint32_t axis, wl_fixed_t value)
{
}

static void
pointer_handle_frame(void *data, struct wl_pointer *wl_pointer)
{
}
static void
pointer_handle_axis_source(void *data, struct wl_pointer *wl_pointer,
		uint32_t axis_source)
{
}
static void
pointer_handle_axis_stop(void *data, struct wl_pointer *wl_pointer,
		uint32_t time, uint32_t axis)
{
}
static void
pointer_handle_axis_discrete(void *data, struct wl_pointer *wl_pointer,
		uint32_t axis, int32_t discrete)
{
}

struct wl_pointer_listener pointer_listener = {
	.enter = pointer_handle_enter,
	.leave = pointer_handle_leave,
	.motion = pointer_handle_motion,
	.button = pointer_handle_button,
	.axis = pointer_handle_axis,

	// Unused, only for satisfying interface
	.frame = pointer_handle_frame,
	.axis_source = pointer_handle_axis_source,
	.axis_stop = pointer_handle_axis_stop,
	.axis_discrete = pointer_handle_axis_discrete,
};

static void
seat_handle_capabilities(void *data, struct wl_seat *seat,
		uint32_t capabilities)
{
	struct Wayland *wl = data;
	if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
		struct wl_pointer *pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(pointer, &pointer_listener, wl);
		wl->pointer.pointer = pointer;
	}
	if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
		struct wl_keyboard *keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(keyboard, &keyboard_listener, wl);
	}
}
static void
seat_handle_name(void *data, struct wl_seat *wl_seat, const char *name)
{
}

static const struct wl_seat_listener seat_listener = {
	.name = seat_handle_name,
	.capabilities = seat_handle_capabilities,
};

static void
handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	struct Wayland *wl = data;
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		uint32_t wantVersion = 4; // for keyboard repeat
		if (wantVersion > version) {
			fprintf(stderr, "error: wl_compositor: want version %d got %d",
					wantVersion, version);
			exit(1);
		}
		wl->compositor = wl_registry_bind(registry, name,
				&wl_compositor_interface, wantVersion);
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		wl->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		wl->xdg_wm_base = wl_registry_bind(registry, name,
				&xdg_wm_base_interface, 1);
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {
		uint32_t wantVersion = 5; // for keyboard repeat
		if (wantVersion > version) {
			fprintf(stderr, "error: wl_seat: want version %d got %d",
					wantVersion, version);
			exit(1);
		}

		struct wl_seat *seat = wl_registry_bind(registry, name,
				&wl_seat_interface, wantVersion);
		wl->seat = seat;
		wl_seat_add_listener(seat, &seat_listener, wl);
	} else if (strcmp(interface, wl_output_interface.name) == 0 &&
			wl->output == NULL) {
		wl->output = wl_registry_bind(registry, name,
				&wl_output_interface, 4);
	} else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
		wl->decor_manager = wl_registry_bind(registry, name,
				&zxdg_decoration_manager_v1_interface, 1);
	}
}

static void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	.global = handle_global,
	.global_remove = handle_global_remove,
};

void
clearBuffer(struct Buffer *buf)
{
	cairo_save(buf->cr);
	cairo_set_source_rgba(buf->cr, 0, 0, 0, 1);
	cairo_set_operator(buf->cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(buf->cr);
	cairo_restore(buf->cr);
}

int allocate_shm_file(size_t size);

struct Buffer
newBuffer(int width, int height, struct wl_shm *shm)
{
	struct Buffer buf = {0};
	int stride = width * 4;

	int shm_pool_size = height * stride;

	buf.width = width;
	buf.height = height;
	buf.stride = stride;
	buf.data_sz = shm_pool_size;

	buf.fd = allocate_shm_file(shm_pool_size);
	if (buf.fd < 0) {
		fprintf(stderr, "Failed to allocate shm file\n");
		exit(1);
	}

	buf.data = mmap(NULL, shm_pool_size, PROT_READ | PROT_WRITE,
			MAP_SHARED, buf.fd, 0);
	if (buf.data == MAP_FAILED) {
		perror("mmap");
		close(buf.fd);
		exit(1);
	}

	struct wl_shm_pool *pool = wl_shm_create_pool(shm, buf.fd, shm_pool_size);
	if (pool == NULL) {
		fprintf(stderr, "Failed to create pool\n");
		close(buf.fd);
		munmap(buf.data, shm_pool_size);
		exit(1);
	}

	buf.wl_buf = wl_shm_pool_create_buffer(pool, 0, width,
			height, stride, WL_SHM_FORMAT_ARGB8888);
	if (buf.wl_buf == NULL) {
		fprintf(stderr, "Failed to create buffer\n");
		exit(1);
	}
	wl_shm_pool_destroy(pool);


	buf.surf = cairo_image_surface_create_for_data(
			(unsigned char *)buf.data, CAIRO_FORMAT_ARGB32,
			width, height, stride);
	if (cairo_surface_status(buf.surf) != CAIRO_STATUS_SUCCESS) {
		fprintf(stderr, "cairo: %s\n",
				cairo_status_to_string(cairo_surface_status(buf.surf)));
		exit(1);
	}
	buf.cr = cairo_create(buf.surf);
	if (cairo_status(buf.cr) != CAIRO_STATUS_SUCCESS) {
		fprintf(stderr, "cairo: %s\n",
				cairo_status_to_string(cairo_status(buf.cr)));
		exit(1);
	}

	cairo_set_font_face(buf.cr, wayland.font_face);

	return buf;
}

void
freeBuffer(struct Buffer *buf)
{
	if (buf->data == NULL) {
		return;
	}
	cairo_surface_finish(buf->surf);
	cairo_surface_destroy(buf->surf);
	cairo_destroy(buf->cr);

	munmap(buf->data, buf->data_sz);

	wl_buffer_destroy(buf->wl_buf);
	close(buf->fd);

	buf->surf = NULL;
	buf->cr = NULL;
	buf->wl_buf = NULL;
	buf->fd = -1;
	buf->data = NULL;
	buf->data_sz = 0;
}

void
platform_InitFont(struct Wayland *wl)
{
	FT_Error err = FT_Init_FreeType(&wl->ft_lib);
	if (err) {
		fprintf(stderr, "error: failed to initalize freetype library: %s\n",
				FT_Error_String(err));
		exit(1);
	}

	char *font_file = GAME_DATA_DIR"/fonts/FreeSerifBoldItalic.ttf";
	err = FT_New_Face(wl->ft_lib, font_file, 0, &wl->ft_face);
	if (err) {
		fprintf(stderr, "error: failed to load file %s: %s\n",
				font_file, FT_Error_String(err));
		exit(1);
	}
	wl->font_face = cairo_ft_font_face_create_for_ft_face(wl->ft_face, 0);
}

void
platform_Init(struct Wayland *wl, bool fullscreen)
{
	wl->display = wl_display_connect(NULL);
	if (wl->display == NULL) {
		fprintf(stderr, "Can't connect to the display\n");
		exit(1);
	}
	struct wl_registry *registry = wl_display_get_registry(wl->display);
	wl_registry_add_listener(registry, &registry_listener, wl);
	wl_display_roundtrip(wl->display);

	if (wl->shm == NULL || wl->compositor == NULL || wl->xdg_wm_base == NULL) {
		fprintf(stderr, "no wl_shm, xdg_wm_base or wl_compositor\n");
		exit(1);
	}

	platform_InitFont(wl);

	wl->buffer = newBuffer(wl->width, wl->height, wl->shm);
	clearBuffer(&wl->buffer);
}

static void
xdg_surface_handle_configure(void *data, struct xdg_surface *xdg_surface,
		uint32_t serial)
{
	struct Wayland *wl = data;
	xdg_surface_ack_configure(xdg_surface, serial);
	wl_surface_commit(wl->surface);
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_handle_configure,
};

static void
xdg_toplevel_handle_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	struct Wayland *wl = data;
	wl->quit = true;
}

static void
xdg_toplevel_handle_configure(void *data, struct xdg_toplevel *xdg_toplevel,
		int32_t width, int32_t height, struct wl_array *states)
{
	struct Wayland *wl = data;
	if (width <= 0)
		wl->width = 640;
	else
		wl->width = width;

	if (height <= 0)
		wl->height = 480;
	else
		wl->height = height;

	wl->configured = true;
	wl->redraw = true;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = xdg_toplevel_handle_configure,
	.close = xdg_toplevel_handle_close,
};

void
xdg_wm_base_handle_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
	xdg_wm_base_pong(xdg_wm_base, serial);
}

struct xdg_wm_base_listener xdg_wm_base_listener = {
	.ping = xdg_wm_base_handle_ping,
};

static void
platform_init_cursor(struct Wayland *wl)
{
	int cursor_size = 24;
	char *env_cursor_size = getenv("XCURSOR_SIZE");
	if (env_cursor_size && *env_cursor_size) {
		char *end;
		int n = strtol(env_cursor_size, &end, 0);
		if (*end == '\0' && n > 0) {
			cursor_size = n;
		}
	}

	char *cursor_theme = getenv("XCURSOR_THEME");
	wl->pointer.theme = wl_cursor_theme_load(cursor_theme, cursor_size, wl->shm);
	for (size_t i = 0; i < sizeof(cursors) / sizeof(cursors[0]); i++) {
		cursors[i].cursor = wl_cursor_theme_get_cursor(wl->pointer.theme, cursors[i].name);
	}
	wl->pointer.cursor = cursors[0].cursor;
	if (cursors[0].cursor == NULL) {
		fprintf(stderr, "error: cursor theme %s doesn't have cursor left_ptr",
				cursor_theme);
		// XXX: maybe we can just disable pointer operations / rendering
		exit(1);
	}
	wl->pointer.image = wl->pointer.cursor->images[0];
	wl->pointer.curimg = 0;
	struct wl_buffer *cursor_buffer = wl_cursor_image_get_buffer(wl->pointer.image);

	wl->pointer.surface = wl_compositor_create_surface(wl->compositor);
	wl_surface_attach(wl->pointer.surface, cursor_buffer, 0, 0);
	wl_surface_damage_buffer(wl->pointer.surface, 0, 0,
			wl->pointer.image->width, wl->pointer.image->height);
	wl_surface_commit(wl->pointer.surface);
}

void
platform_Open(struct Wayland *wl)
{
	wl->surface = wl_compositor_create_surface(wl->compositor);
	wl->xdg_surface = xdg_wm_base_get_xdg_surface(wl->xdg_wm_base, wl->surface);
	wl->xdg_toplevel = xdg_surface_get_toplevel(wl->xdg_surface);

	xdg_surface_add_listener(wl->xdg_surface, &xdg_surface_listener, wl);
	xdg_toplevel_add_listener(wl->xdg_toplevel, &xdg_toplevel_listener, wl);
	xdg_wm_base_add_listener(wl->xdg_wm_base, &xdg_wm_base_listener, wl);

	xdg_toplevel_set_title(wl->xdg_toplevel, NAME);
	xdg_toplevel_set_app_id(wl->xdg_toplevel, NAME);

	if (wl->decor_manager != NULL) {
		wl->top_decor =
			zxdg_decoration_manager_v1_get_toplevel_decoration(
					wl->decor_manager, wl->xdg_toplevel);
		zxdg_toplevel_decoration_v1_set_mode(
				wl->top_decor,
				ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
	}

	platform_init_cursor(wl);

	wl_surface_commit(wl->surface);
	wl_display_roundtrip(wl->display);

	wl_surface_attach(wl->surface, wl->buffer.wl_buf, 0, 0);
	wl_surface_damage_buffer(wl->surface, 0, 0, UINT32_MAX, UINT32_MAX);
	wl_surface_commit(wl->surface);
}

static void wl_surface_frame_done(void *data, struct wl_callback *cb, uint32_t time);

static struct wl_callback_listener wl_surface_frame_listener = {
	.done = wl_surface_frame_done,
};

static void
wl_surface_frame_done(void *data, struct wl_callback *cb, uint32_t time)
{
	static uint32_t prevTime = 0;
	wl_callback_destroy(cb);

	struct Wayland *wl = data;
	cb = wl_surface_frame(wl->surface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, wl);

	if (wl->configured) {
		struct Buffer prev = wl->buffer;
		wl->buffer = newBuffer(wl->width, wl->height, wl->shm);
		freeBuffer(&prev);

		wl->configured = false;
	}
	double dt = (time - prevTime) / 1000.0;
	if (prevTime == 0) {
		dt = 1.0 / 60.0;
	}
	prevTime = time;

	wl->game_input.ptr_x = wl->pointer.x;
	wl->game_input.ptr_y = wl->pointer.y;

	if (!game_UpdateAndDraw(dt, wl->game_input, wl->width, wl->height)) {
		wl->quit = true;
	}
	for (int player = 0; player < 2; player++) {
		for (int i = 0; i < KEY_COUNT; i++) {
			switch (wl->game_input.keys[player][i]) {
			case KEY_PRESSED:
				wl->game_input.keys[player][i] = KEY_PRESSED_REPEAT;
				break;
			case KEY_RELEASED:
				wl->game_input.keys[player][i] = KEY_UNKNOWN;
				break;
			default: // ignore
				break;
			}
		}
	}

	wl_surface_attach(wl->surface, wl->buffer.wl_buf, 0, 0);
	wl_surface_damage_buffer(wl->surface, 0, 0, wl->width, wl->height);
	wl_surface_commit(wl->surface);
}

void
run(struct Wayland *wl)
{
	while (!wl->quit && wl_display_dispatch(wl->display) != -1) {
		// noop
	}
}

int
main(int argc, char *argv[])
{
	int x;
	bool fullscreen = false;
	if (argc > 1) {
		while ((x = getopt(argc, argv, "vlf")) != -1) {
			switch (x) {
			case 'v':
				puts(NAME": " VERSION);
				return 0;
			case 'l':
				listFunc();
				return 0;
			case 'f':
				fullscreen = true;
				break;
			default:
				fputs("Usage: fuyunix [-v|-l|-f]\n", stderr);
				return 1;
			}
		}
	}

	struct Wayland *wl = &wayland;

	wl->width = LOGICAL_WIDTH;
	wl->height = LOGICAL_HEIGHT;

	platform_Init(wl, fullscreen);

	game_Init();

	platform_Open(wl);

	struct wl_callback *cb = wl_surface_frame(wl->surface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, wl);

	loadConfig();

	run(wl);

	game_Quit();

	freeBuffer(&wl->buffer);
	xdg_toplevel_destroy(wl->xdg_toplevel);
	xdg_surface_destroy(wl->xdg_surface);
	xdg_wm_base_destroy(wl->xdg_wm_base);
	wl_surface_destroy(wl->surface);
	wl_display_disconnect(wl->display);

	cairo_font_face_destroy(wl->font_face);
	FT_Done_Face(wl->ft_face);
	FT_Done_Library(wl->ft_lib);

	return 0;
}
