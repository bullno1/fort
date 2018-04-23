#ifndef FORT_USE_EXTERNAL_BK
#define BK_IMPLEMENTATION
#endif

#include <stdlib.h>
#include <bk/stdstream.h>
#include <bk/default_allocator.h>
#include <bk/printf.h>
#include <fort.h>
#include <fort/utils.h>

static fort_err_t
fort_main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;

	fort_err_t err;
	fort_ctx_t* ctx = NULL;
	fort_t* fort = NULL;

	fort_ctx_config_t ctx_cfg = {
		.allocator = bk_default_allocator
	};
	fort_config_t cfg = {
		.output = bk_stdout,
		.param_stack_size = 1024,
		.return_stack_size = 1024
	};

	if((err = fort_create_ctx(&ctx_cfg, &ctx)) != FORT_OK) { goto end; }
	if((err = fort_create(ctx, &cfg, &fort)) != FORT_OK) { goto end; }
	if((err = fort_load_builtins(fort)) != FORT_OK) { goto end; }

	err = fort_interpret(fort, bk_stdin, FORT_STRING_REF("stdin"));

end:
	if(fort != NULL) { fort_destroy(fort); }
	if(ctx != NULL) { fort_destroy_ctx(ctx); }

	return err;
}

int
main(int argc, char* argv[])
{
	fort_err_t err = fort_main(argc, argv);
	bk_printf(bk_stdout, "%s\n", fort_err_t_to_str(err) + sizeof("FORT"));

	return err == FORT_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
