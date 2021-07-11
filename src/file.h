int readSaveFile(void);
void writeSaveFile(int level);

struct List {
	int player;
	char *funcname;
	char *key;
};

void writeKeysFile(struct List *list, int lim);
