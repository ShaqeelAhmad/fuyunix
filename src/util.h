void *qrealloc(void *ptr, size_t size);
void *qcalloc(size_t nmemb, size_t size);
int readSaveFile(void);
void writeSaveFile(int level);
char *readKeyConf(char *filename);
char *getPath(char *fullpath, char *xdg, char *file, int createDir);
