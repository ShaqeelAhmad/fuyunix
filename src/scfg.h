/* Look at LICENSE.scfg for details */
#ifndef SCFG_H
#define SCFG_H

#include <stddef.h>
#include <stdio.h>

struct scfg_block {
	struct scfg_directive *directives;
	size_t directives_len;
};

struct scfg_directive {
	char *name;

	char **params;
	size_t params_len;

	struct scfg_block children;

	int lineno;
};

int scfg_load_file(struct scfg_block *block, const char *path);
int scfg_parse_file(struct scfg_block *block, FILE *f);
void scfg_block_finish(struct scfg_block *block);

#endif
