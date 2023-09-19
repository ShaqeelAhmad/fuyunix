void *erealloc(void *ptr, size_t size);
void *ecalloc(size_t nmemb, size_t size);
int readSaveFile(void);
void writeSaveFile(int level);
char *readKeyConf(char *filename);
int getConfigDir(char *path, size_t path_len);
bool getConfigFile(char *path, size_t path_len);
