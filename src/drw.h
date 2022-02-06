#ifndef _DRW_H_
#define _DRW_H_

void drwMenu(int player);
void init(int winFlags);
void cleanup(void);
void drw(void);
void homeMenu(void);

void left(int i);
void right(int i);
void down(int i);
void jump(int i);

#endif /* _DRW_H_ */
