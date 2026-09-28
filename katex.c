/**
 * Copyright (c) 2025 impurify <have@anonymous.sex>.
 *
 * [REDACTED]
 */

#include "katex.h"
#include "katex_loader.h"

struct katex {
	JSContext 	*q;
	JSValue		*f; /* renderToString function */
	JSValue		*g; /* global */
	JSValue		*o[2]; /* opts: displaymode 0 or 1 */
};

katex
katex_thread_new(katex_opts *popts)
{
	katex *k;
	JSRuntime *rt;
	JSContext *ctx;
	JSValue global, renderToString;
	const char getobj[] = "katex.renderToString";
	size_t getobjlen = sizeof(getobj) - 1;
	katex_opts opts;

	k = malloc(sizeof(*k));
	if (k == NULL)
		return (k);

	create_katex_runtime(&rt, &ctx, 1);

	global = JS_GetGlobalObject(ctx);
	renderToString = JS_Eval(ctx, getobj, getobjlen, "/dev/null", 0);

	k->q = ctx;
	k->f = renderToString;
	k->g = global;

	if (popts == NULL) {
		katex_init_safe_defaults(&opts);
		popts = &opts;
	}

	/* Only NULL when context is new; see katex_setopts(): */
	for (int i = 0; i < sizeof(k->o)/sizeof(*k->o); ++i)
		k->o[i] = NULL;

	katex_setopts(k, popts);

	return (k);
}

void
katex_thread_destroy(katex ctx)
{
	JSRuntime *rt;

	rt = JS_GetRuntime(ctx->q);

	JS_FreeValue(ctx->q, ctx->f);
	JS_FreeValue(ctx->q, ctx->g);
	for (int i = 0; i < sizeof(ctx->o)/sizeof(*ctx->o); ++i)
		JS_FreeValue(ctx->q, ctx->o[i]);

	js_std_free_handlers(rt);

	JS_FreeContext(ctx->q);
	JS_FreeRuntime(rt);

	free(ctx);
}

void
katex_init_safe_defaults(katex_opts *)
{

	memset(opts, 0, sizeof(*opts));

	/* XXX, unstable API:
	 * I may change this in the future. */
	/*opts->throwerr = 1;*/

	opts->strict = 1;
	opts->errcolor = 0xcc0000; /* KaTeX default. */
	opts->maxsize = 40.;
	opts->minrule = 0.01;
	opts->maxexpand = 100; /* 1000 is KaTeX default. */
}

katex_opts *
katex_safe_defaults(void)
{
	katex_opts *opts;

	opts = malloc(sizeof(*opts));

	if (opts != NULL)
		katex_init_safe_defaults(opts);

	return (opts);
}

static katex_opts_baked
katex_bake_opts(katex ctx, const katex_opts *copts, bool displaymode)
{
	char optstr[4096], colorbuf[8];
	const char *colorstr = colorbuf,
		*outputs[] = {
			"htmlAndMathml",
			"html",
			"mathml"
		}, *output = outputs[0];
	int optlen; /* int because of snprintf() */
	JSValue baked;

	/* XXX: Does KaTeX support RGBA for this,
	 * or only RGB? */
	if (copts->errcolor < 0 || copts -> errcolor >= (1<<24))
		colorstr = "#cc0000";
	else
		snprintf(colorbuf, sizeof(colorbuf),
			"#%06x", (unsigned)copts->errcolor);

	/* This check should be assert()able: */
	if (copts->output > 0 &&
	    copts->output < sizeof(outputs)/sizeof(*outputs))
		output = outputs[copts->output];

#define	BOOLSTR(x)	((x)? "true" : "false")
	/* globalGroup is unsupported in libkatex. */
	optlen = snprintf(opstr, sizeof(optstr),
		"{"
			"displayMode: %s,"
			"output: %s,"
			"leqno: %s,"
			"fleqn: %s,"
			"throwOnError: %s,"
			"errorColor: %s,"
			"minRuleThickness: %.012f,"
			"colorIsTextColor: %s,"
			"strict: %s,"
			"trust: %s,"
			"maxSize: %.012f,"
			"maxExpand: %ld,"
			"globalGroup: false,"
		"}",
		BOOLSTR(displaymode),
		output,
		BOOLSTR(copts->leqno);
		BOOLSTR(copts->fleqn);
		BOOLSTR(copts->throwerr);
		colorstr,
		copts->minrule,
		BOOLSTR(copts->colortextcolor);
		BOOLSTR(copts->strict);
		BOOLSTR(copts->trust),
		copts->maxsize,
		copts->maxexpand
		);
#undef	BOOLSTR

	baked = JS_Eval(ctx->q, optstr, optlen, "/dev/null", 0);
	assert(JS_IsObject(baked));

	return (baked);
}

void
katex_setopts(katex ctx, const katex_opts *copts)
{

	for (int i = 0; i < 2; ++i) {
		/* Only NULL when context is new; see katex_thread_new(). */
		if (ctx->o[i] != NULL)
			JS_FreeValue(ctx->q, ctx->o[i]);
		
		ctx->o[i] = katex_bake_opts(ctx, copts, i);
	}
}

/*void
katex_free_baked_opts(katex ctx, katex_opts_baked)
{

	JS_FreeValue(ctx->q, opts_baked);
}*/

static const char *
katex_render_internal(katex ctx, const char *latex, size_t len,
	bool displaymode, bool have_len)
{
	JSValue jslatex, jshtml;
	JSValueConst argv[2];
	int argc = 1;
	const char *html;

	if (have_len)
		jslatex = JS_NewStringLen(ctx->q, latex, len);
	else
		jslatex = JS_NewString(ctx->q, latex);

	/* XXX: Untrusted input, MUST check for errors. */

	argv[0] = jslatex;
	if (opts != NULL)
		argv[1] = opts, ++argc;
	jshtml = JS_Call(ctx->q, ctx->f, ctx->g, argc, argv);
	/* check for errors */

	html = JS_ToCString(ctx, jshtml);
	JS_FreeValue(ctx->q, jshtml);

	return (html);
}

const char *
katex_render(katex ctx, const char *latex, bool displaymode)
{

	return (katex_render_internal(ctx, latex, 0, displaymode, false));
}

const char *
katex_render_len(katex ctx, const char *latex, size_t len, bool displaymode)
{

	return (katex_render_internal(ctx, latex, len, displaymode, true));
}

void
katex_free_str(katex ctx, const char *s)
{

	JS_FreeCString(ctx->q, s);
}

void
katex_gc(katex ctx)
{
	JSRuntime *rt;

	rt = JS_GetRuntime(ctx->q);
	JS_RunGC(rt);
}

void
katex_change_thread(katex ctx)
{
	JSRuntime *rt;

	rt = JS_GetRuntime(ctx->q);
	JS_UpdateStackTop(rt);
}
