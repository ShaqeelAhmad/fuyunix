#ifndef _GAME_H_
#define _GAME_H_

#ifndef GAME_DATA_DIR
#define GAME_DATA_DIR "./data"
#endif

#define GRAVITY 98.0f
#define TERMINAL_VELOCITY (GRAVITY * 1.2)
#define JUMP_ACCEL 8
#define SPEED_ACCEL 400.0f
#define SPEED_MAX 800
#define FRICTION 0.91f
#define PROJ_DX 380.0f
#define PROJ_RET 500
#define PROJ_SIZE 12

#define FRAME_NUM 3

#define BLOCK_SIZE 32
#define PLAYER_SIZE BLOCK_SIZE
#define LOGICAL_WIDTH 900
#define LOGICAL_HEIGHT 450
#define MAX_STAGE_HEIGHT (LOGICAL_HEIGHT * 4)
#define MAX_STAGE_LENGTH (LOGICAL_WIDTH * 10)

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

struct game_Input {
	KeyState keys [2][KEY_COUNT];
	int ptr_x;
	int ptr_y;
};

struct game_Rect {
	int x, y, w, h;
};
struct game_FRect {
	float x, y, w, h;
};

typedef struct game_Texture game_Texture;

struct game_Color {
	uint8_t r, g, b, a;
};

struct game_Data {
	int level;
};

#define GAME_RGBA(r, g, b, a) (struct game_Color){r, g, b, a}
#define GAME_RGB(r, g, b) (struct game_Color){r, g, b, 0xFF}
#define GAME_BLACK (struct game_Color){0, 0, 0, 0xFF}

void game_Init(void);
bool game_UpdateAndDraw(double dt, struct game_Input input, int width, int height);
void game_Quit(void);

game_Texture *platform_LoadTexture(char *file);
void platform_DestroyTexture(game_Texture *t);
void platform_RenderText(char *text, int size, struct game_Color fg, int x, int y);
void platform_MeasureText(char *text, int size, int *w, int *h);
void platform_FillRect(struct game_Color c, struct game_Rect *rect);
void platform_DrawTexture(game_Texture *t, struct game_Rect *src, struct game_Rect *dst);
void platform_Clear(struct game_Color c);
void platform_DrawLine(struct game_Color c, int x1, int y1, int x2, int y2);
bool platform_ReadSaveData(struct game_Data *data);
void platform_WriteSaveData(struct game_Data *data);
// XXX: have different levels of logging? e.g error, warning, debug.
#if defined(__GNUC__)
void __attribute__((format (printf, 1, 2))) platform_Log(char *fmt, ...);
#else
void platform_Log(char *fmt, ...);
#endif

#endif /* _GAME_H_ */
