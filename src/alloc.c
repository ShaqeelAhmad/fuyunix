#include <stdio.h>
#include <stdlib.h>

void *
qcalloc(size_t nmemb, size_t size)
{
	void *p = calloc(nmemb, size);
	if (p == NULL) {
		perror("Unable to allocate memory");
		exit(1);
	}

	return p;
}

void *
qrealloc(void *ptr, size_t size)
{
	void *p = realloc(ptr, size);
	if (p == NULL) {
		perror("Unable to reallocate memory");
		exit(1);
	}

	return p;
}
