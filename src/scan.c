#include "internal.h"
#include <ctype.h>
#include <errno.h>
#include <bk/array.h>
#include <bk/fs.h>

fort_err_t
fort_next_char(fort_t* fort, char* ch)
{
	size_t len = 1;
	char current_char;
	fort_state_t* state = &fort->state;

	int err = bk_fread(state->input, &current_char, &len);

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
		char last_char = state->last_char;
		int is_crlf = state->last_char == '\r' && current_char == '\n';
		int is_newline = last_char == '\r' || last_char == '\n';

		if(is_newline && !is_crlf)
		{
			state->location.column = 1;
			state->location.line += 1;
		}
		else
		{
			state->location.column += 1;
		}

		state->last_char = current_char;
		*ch = current_char;
		return FORT_OK;
	}
}

fort_err_t
fort_next_token(fort_t* fort, fort_token_t* token)
{
	char ch;
	fort_err_t err;
	fort_location_t token_start, token_end;

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
			token_start = fort->state.location;
			token_end = fort->state.location;
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
			token_end = fort->state.location;
		}
	}

	*token = (fort_token_t) {
		.lexeme = {
			.ptr = fort->scan_buf,
			.length = bk_array_len(fort->scan_buf)
		},
		.location = {
			.start = token_start,
			.end = token_end
		}
	};
	bk_array_push(fort->scan_buf, '\0'); // NULL-terminate

	return FORT_OK;
}
