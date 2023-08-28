#ifndef _GAME_H_
#define _GAME_H_

enum KeySym {
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
};

typedef struct {
	bool isKeyPressed[KEY_COUNT];
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
void game_Update(game_Input input);
bool game_HasIntersectionF(game_FRect a, game_FRect b);
bool game_Draw(double dt);
void game_Quit(void);

game_Texture *platform_LoadTexture(char *file);
void platform_DestroyTexture(game_Texture *t);
double platform_GetFPS(void);
void platform_LoadConfig(void);
void platform_RenderText(char *text, int size, game_Color fg, int x, int y);
void platform_MeasureText(char *text, int *w, int *h);
void platform_FillRects(game_Color c, game_Rect *rects, int count);
void platform_FillRect(game_Color c, game_Rect *rect);
void platform_DrawTexture(game_Texture *t, game_Rect *src, game_Rect *dst);
void platform_Clear(game_Color c);
void platform_DrawLine(game_Color c, int x1, int y1, int x2, int y2);

#endif /* _GAME_H_ */
