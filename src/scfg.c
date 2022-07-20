/* Look at LICENSE.scfg for details */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "scfg.h"

struct scfg_buffer {
	void *data;
	size_t len, cap;
};

static int buffer_append(struct scfg_buffer *buf, const void *src, size_t size) {
	if (buf->len + size > buf->cap) {
		size_t new_cap = buf->cap;
		if (new_cap == 0) {
			new_cap = 128;
		}
		while (buf->len + size > new_cap) {
			new_cap *= 2;
		}
		char *new_data = realloc(buf->data, new_cap);
		if (new_data == NULL) {
			return -errno;
		}
		buf->cap = new_cap;
		buf->data = new_data;
	}

	void *dst = (char *)buf->data + buf->len;
	memcpy(dst, src, size);
	buf->len += size;
	return 0;
}

static int buffer_append_char(struct scfg_buffer *buf, char ch) {
	return buffer_append(buf, &ch, 1);
}

static void buffer_finish(struct scfg_buffer *buf) {
	free(buf->data);
	memset(buf, 0, sizeof(*buf));
}

static void *buffer_steal(struct scfg_buffer *buf) {
	void *data = buf->data;
	memset(buf, 0, sizeof(*buf));
	return data;
}

struct scfg_parser {
	FILE *f;
	int prev_ch;
	int lineno;
};

static int parser_read_char(struct scfg_parser *parser) {
	errno = 0;
	int ch = fgetc(parser->f);
	if (ch == EOF) {
		parser->prev_ch = '\0';
		if (errno != 0) {
			return -errno;
		}
		return '\0';
	}
	parser->prev_ch = ch;
	if (ch == '\n') {
		parser->lineno++;
	}
	return ch;
}

static void parser_unread_char(struct scfg_parser *parser) {
	assert(parser->prev_ch >= 0);
	if (parser->prev_ch == 0) {
		return;
	}
	ungetc(parser->prev_ch, parser->f);
	if (parser->prev_ch == '\n') {
		parser->lineno--;
	}
	parser->prev_ch = -1;
}

static void parser_consume_whitespace(struct scfg_parser *parser) {
	while (1) {
		switch (parser_read_char(parser)) {
		case ' ':
		case '\t':
			break;
		default:
			parser_unread_char(parser);
			return;
		}
	}
}

static void parser_consume_line(struct scfg_parser *parser) {
	while (1) {
		int ch = parser_read_char(parser);
		if (ch < 0 || ch == '\0' || ch == '\n') {
			return;
		}
	}
}

static int parser_read_atom(struct scfg_parser *parser, char **str_ptr) {
	struct scfg_buffer buf = {0};
	while (1) {
		int ch = parser_read_char(parser);
		if (ch < 0) {
			return ch;
		}

		switch (ch) {
		case '\0':
		case ' ':
		case '\t':
		case '\n':
			parser_unread_char(parser);
			buffer_append_char(&buf, '\0');
			*str_ptr = buffer_steal(&buf);
			return 0;
		case '"':
		case '\'':
		case '{':
		case '}':
			buffer_finish(&buf);
			fprintf(stderr, "scfg: unexpected '%c' in atom\n", ch);
			return -EINVAL;
		default:
			buffer_append_char(&buf, ch);
			break;
		}
	}
}

static int parser_read_dquote_word(struct scfg_parser *parser, char **str_ptr) {
	struct scfg_buffer buf = {0};
	while (1) {
		int ch = parser_read_char(parser);
		if (ch < 0) {
			return ch;
		}

		switch (ch) {
		case '\0':
		case '\n':
			buffer_finish(&buf);
			fprintf(stderr, "scfg: unterminated double-quoted string\n");
			return -EINVAL;
		case '"':
			buffer_append_char(&buf, '\0');
			*str_ptr = buffer_steal(&buf);
			return 0;
		case '\\':
			ch = parser_read_char(parser);
			if (ch < 0) {
				return ch;
			} else if (ch == '\n') {
				fprintf(stderr, "scfg: can't escape '\\n' in double-quoted string\n");
				return -EINVAL;
			}
			/* fallthrough */
		default:
			buffer_append_char(&buf, ch);
			break;
		}
	}
}

static int parser_read_squote_word(struct scfg_parser *parser, char **str_ptr) {
	struct scfg_buffer buf = {0};
	while (1) {
		int ch = parser_read_char(parser);
		if (ch < 0) {
			return ch;
		}

		switch (ch) {
		case '\0':
		case '\n':
			buffer_finish(&buf);
			fprintf(stderr, "scfg: unterminated single-quoted string\n");
			return -EINVAL;
		case '\'':
			buffer_append_char(&buf, '\0');
			*str_ptr = buffer_steal(&buf);
			return 0;
		default:
			buffer_append_char(&buf, ch);
			break;
		}
	}
}

static int parser_read_word(struct scfg_parser *parser, char **str_ptr) {
	int ch = parser_read_char(parser);
	if (ch < 0) {
		return ch;
	}

	switch (ch) {
	case '"':
		return parser_read_dquote_word(parser, str_ptr);
	case '\'':
		return parser_read_squote_word(parser, str_ptr);
	default:
		parser_unread_char(parser);
		return parser_read_atom(parser, str_ptr);
	}
}

static int parser_read_block(struct scfg_parser *parser, struct scfg_block *block, bool *closing_brace);

static int parser_read_directive(struct scfg_parser *parser, struct scfg_directive *dir) {
	memset(dir, 0, sizeof(*dir));

	dir->lineno = parser->lineno;

	int res = parser_read_word(parser, &dir->name);
	if (res != 0) {
		return res;
	}
	parser_consume_whitespace(parser);

	struct scfg_buffer params_buf = {0};
	while (1) {
		int ch = parser_read_char(parser);
		if (ch < 0) {
			return ch;
		} else if (ch == '\0' || ch == '\n') {
			break;
		} else if (ch == '{') {
			bool closing_brace = false;
			res = parser_read_block(parser, &dir->children, &closing_brace);
			if (res != 0) {
				return res;
			} else if (!closing_brace) {
				fprintf(stderr, "scfg: expected '}'\n");
				return -EINVAL;
			}
			break;
		}

		switch (ch) {
		case '}':
			fprintf(stderr, "scfg: unexpected '}'\n");
			return -EINVAL;
		default:
			parser_unread_char(parser);
			char *param = NULL;
			res = parser_read_word(parser, &param);
			if (res != 0) {
				return res;
			}
			buffer_append(&params_buf, &param, sizeof(param));
			parser_consume_whitespace(parser);
			break;
		}
	}
	dir->params_len = params_buf.len / sizeof(char *);
	dir->params = buffer_steal(&params_buf);

	return 0;
}

static int parser_read_block(struct scfg_parser *parser, struct scfg_block *block, bool *closing_brace) {
	memset(block, 0, sizeof(*block));
	*closing_brace = false;

	struct scfg_buffer dirs_buf = {0};
	while (1) {
		parser_consume_whitespace(parser);

		int ch = parser_read_char(parser);
		if (ch < 0) {
			return ch;
		} else if (ch == '\n') {
			continue;
		} else if (ch == '#') {
			parser_consume_line(parser);
			continue;
		} else if (ch == 0 || ch == '}') {
			*closing_brace = ch == '}';
			break;
		}
		parser_unread_char(parser);

		struct scfg_directive dir = {0};
		int res = parser_read_directive(parser, &dir);
		if (res != 0) {
			return res;
		}
		buffer_append(&dirs_buf, &dir, sizeof(dir));
	}
	block->directives_len = dirs_buf.len / sizeof(struct scfg_directive);
	block->directives = buffer_steal(&dirs_buf);

	return 0;
}

int scfg_load_file(struct scfg_block *block, const char *path) {
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		return -1;
	}

	int res = scfg_parse_file(block, f);
	fclose(f);
	return res;
}

int scfg_parse_file(struct scfg_block *block, FILE *f) {
	struct scfg_parser parser = { .f = f, .lineno = 1 };
	bool closing_brace = false;
	int res = parser_read_block(&parser, block, &closing_brace);
	if (res != 0) {
		return res;
	} else if (closing_brace) {
		fprintf(stderr, "scfg: unexpected '}'\n");
		return -EINVAL;
	}
	return 0;
}

static void directive_finish(struct scfg_directive *dir) {
	free(dir->name);
	for (size_t i = 0; i < dir->params_len; i++) {
		free(dir->params[i]);
	}
	free(dir->params);
	scfg_block_finish(&dir->children);
}

void scfg_block_finish(struct scfg_block *block) {
	for (size_t i = 0; i < block->directives_len; i++) {
		directive_finish(&block->directives[i]);
	}
	free(block->directives);
}
