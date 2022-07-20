#ifndef _KEYS_H_
#define _KEYS_H_

void handleKeys(void);
void menuHandleKeys(int scancode);
void freeKeys(void);
void listFunc(void);
void loadConfig(void);

enum KeySym {
	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_PAUSE,
	KEY_QUIT,
	KEY_SELECT,
	KEY_COUNT,
};

#endif /* _KEYS_H_ */
