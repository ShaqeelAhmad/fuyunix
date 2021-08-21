#ifndef _DRW_H_
#define _DRW_H_

void drwmenu(int player);
void init(void);
void cleanup(void);
void drw(void);
void homeMenu(void);

void left(int i);
void right(int i);
void down(int i);
void jump(int i);

/* NOTE: Maybe put this in Makefile and use -D to define it */
/* #define RESOURCE_PATH "/usr/local/games/fuyunix" */
#define RESOURCE_PATH ".."

#endif /* _DRW_H_ */
