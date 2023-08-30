#ifndef _GAME_H_
#define _GAME_H_

typedef enum {
	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_PAUSE,
	KEY_QUIT,
	KEY_SHOOT,

	/* KEY_SELECT key isn't configurable */
	KEY_SELECT,

	KEY_COUNT,
} KeySym;

typedef enum {
	KEY_UNKNOWN,
	KEY_PRESSED,
	KEY_PRESSED_REPEAT,
	KEY_RELEASED,
} KeyState;

typedef enum {
	CURSOR_ARROW,
	CURSOR_COUNT,
} Cursor;

typedef struct {
	KeyState keys [2][KEY_COUNT];
	int ptr_x;
	int ptr_y;
} game_Input;

typedef struct {
	int x, y, w, h;
} game_Rect;
typedef struct {
	float x, y, w, h;
} game_FRect;

typedef struct game_Texture game_Texture;

typedef struct {
	uint8_t r, g, b, a;
} game_Color;

#define GAME_RGBA(r, g, b, a) (game_Color){r, g, b, a}
#define GAME_RGB(r, g, b) (game_Color){r, g, b, 0xFF}
#define GAME_BLACK (game_Color){0, 0, 0, 0xFF}

void game_Init(void);
bool game_UpdateAndDraw(double dt, game_Input input, int width, int height);
void game_Quit(void);

game_Texture *platform_LoadTexture(char *file);
void platform_DestroyTexture(game_Texture *t);
void platform_RenderText(char *text, int size, game_Color fg, int x, int y);
void platform_MeasureText(char *text, int size, int *w, int *h);
void platform_FillRects(game_Color c, game_Rect *rects, int count);
void platform_FillRect(game_Color c, game_Rect *rect);
void platform_DrawTexture(game_Texture *t, game_Rect *src, game_Rect *dst);
void platform_Clear(game_Color c);
void platform_DrawLine(game_Color c, int x1, int y1, int x2, int y2);

#endif /* _GAME_H_ */
