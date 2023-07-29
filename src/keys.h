#ifndef _KEYS_H_
#define _KEYS_H_

void handleKeys(void);
void menuHandleKeys(SDL_Scancode scancode);
void freeKeys(void);
void listFunc(void);
void loadConfig(void);
void handleKeyups(SDL_Scancode scancode);


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
#endif /* _KEYS_H_ */
