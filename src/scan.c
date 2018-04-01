#include "internal.h"
#include <ctype.h>
#include <errno.h>
#include <bk/array.h>
#include <bk/fs.h>
#include <stdio.h>
fort_err_t
fort_next_char(fort_t* fort, char* ch)
{
	size_t len = 1;
	int err = bk_fread(fort->interpreter_state.input, ch, &len);

	if(len == 0)
	{
		return FORT_ERR_NOT_FOUND;
	}
	else if(err == ENOMEM)
	{
		return FORT_ERR_OOM;
	}
	else if(err != 0)
	{
		return FORT_ERR_IO;
	}
	else
	{
		return FORT_OK;
	}
}

fort_err_t
fort_next_token(fort_t* fort, fort_token_t* token)
{
	char ch;
	fort_err_t err;

	bk_array_clear(fort->scan_buf);

	for(;;)
	{
		if((err = fort_next_char(fort, &ch)) != FORT_OK) { return err; }

		if(isspace(ch))
		{

			continue;
		}
		else
		{
			bk_array_push(fort->scan_buf, ch);
			break;
		}
	}

	for(;;)
	{
		err = fort_next_char(fort, &ch);
		if(err == FORT_ERR_NOT_FOUND)
		{
			break;
		}
		else if(err != FORT_OK)
		{
			return err;
		}

		if(isspace(ch))
		{
			break;
		}
		else
		{
			bk_array_push(fort->scan_buf, ch);
		}
	}

	token->length = bk_array_len(fort->scan_buf);
	token->lexeme = fort->scan_buf;

	return FORT_OK;
}
