#ifndef _GAME_H_
#define _GAME_H_

#define MAX_PLAYERS 2

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

#define PI 3.14159265358979323846264338327950288419716939937510582097494459

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

struct game_V2 {
	int x, y;
};
struct game_Rect {
	int x, y, w, h;
};
struct game_FRect {
	float x, y, w, h;
};

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
bool game_UpdateAndDraw(cairo_t *cr, double dt, struct game_Input input, int width, int height);
void game_Quit(void);

#endif /* _GAME_H_ */
