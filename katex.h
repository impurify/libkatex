/**
 * Copyright (c) 2025 impurify <have@anonymous.sex>.
 *
 * [REDACTED]
 */

#ifndef _KATEX_H_
#define	_KATEX_H_

#include <stdbool.h>

typedef struct katex *katex;

enum KatexOutput {
	KATEX_MATHML_HTML,
	KATEX_HTML,
	KATEX_MATHML
};

/*
 * libkatex handles displaymode differently; RTFM.
 */
typedef struct katex_opts {
	/*unsigned		displaymode : 1;*/
	unsigned		leqno : 1;
	unsigned		fleqn : 1;
	unsigned		throwerr : 1;
	unsigned		colortextcolor : 1;
	unsigned		strict : 1;
	unsigned		trust : 1;

	enum KatexOutput	output;
	long			errcolor;
	double			maxsize; /* HTML em units */
	double			minrule; /* HTML em units */
	long			maxexpand;
} katex_opts;

typedef struct katex_opts_baked *katex_opts_baked;

/* opts may be NULL, in which case safe defaults will be used. */
katex katex_thread_new(katex_opts *);
void katex_thread_destroy(katex);

/*
 * Option handling workflow:
 *
 * 0. Make a C struct with desired options.
 * 1. Bake the options on a context.
 * 2. Reuse baked options as many times as you need.
 * 3. Free baked options on the same context.
 */

void katex_init_safe_defaults(katex_opts *);
/* Pass result to free(): */
katex_opts *katex_safe_defaults(void);

/* Free results with special function on same context: */
katex_opts_baked katex_bake_opts(katex, const katex_opts *);
void katex_free_baked_opts(katex, katex_opts_baked);

/*
 * NULL may be passed for options.
 * Pass result to katex_free_str() with same context.
 */
const char *katex_render(katex, const char *, bool);
const char *katex_render_len(katex, const char *, size_t, bool);

void katex_free_str(katex, const char *);

/* Force GC, usually not necessary: */
void katex_gc(katex);

/*
 * If a context object is passed between threads,
 * call this on th new thread:
 */
void katex_change_thread(katex);

#endif /* !_KATEX_H_ */
