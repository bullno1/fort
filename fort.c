#include <stdlib.h>
#include <bk/stdstream.h>
#include <bk/default_allocator.h>
#include <bk/printf.h>
#include <fort.h>
#include <fort-utils.h>

static fort_err_t
fort_main(fort_config_t* cfg)
{
	fort_err_t err;
	fort_t* fort = NULL;

	if((err = fort_create(cfg, &fort)) != FORT_OK) { goto end; }
	if((err = fort_load_builtins(fort)) != FORT_OK) { goto end; }

	err = fort_interpret(fort, bk_stdin, FORT_STRING_REF("stdin"));

end:
	if(fort != NULL) { fort_destroy(fort); }

	return err;
}

int
main(void)
{
	fort_config_t cfg = {
		.allocator = bk_default_allocator,
		.output = bk_stdout
	};

	fort_err_t err = fort_main(&cfg);
	bk_printf(cfg.output, "%s\n", fort_err_t_to_str(err) + sizeof("FORT"));

	return err == FORT_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
