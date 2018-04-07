#ifndef FORT_H
#define FORT_H

#include <stdint.h>
#include <stddef.h>
#include <bk/macro.h>

#if FORT_DYNAMIC == 1
#	if FORT_BUILD == 1
#		define FORT_DECL BK_EXTERN BK_DYNAMIC_EXPORT
#	else
#		define FORT_DECL BK_EXTERN BK_DYNAMIC_IMPORT
#	endif
#else
#	define FORT_DECL BK_EXTERN
#endif

#ifndef FORT_REAL_TYPE
#define FORT_REAL_TYPE double
#define FORT_STR_TO_REAL strtod
#endif

#ifndef FORT_INT_TYPE
#define FORT_INT_TYPE int64_t
#define FORT_STR_TO_INT strtoll
#endif

struct bk_allocator_s;
struct bk_file_s;

typedef FORT_REAL_TYPE fort_real_t;
typedef FORT_INT_TYPE fort_int_t;
typedef union fort_cell_u fort_cell_t;
typedef struct fort_string_ref_s fort_string_ref_t;
typedef struct fort_s fort_t;
typedef struct fort_word_s fort_word_t;
typedef struct fort_config_s fort_config_t;
typedef struct fort_location_s fort_location_t;
typedef struct fort_loc_range_s fort_loc_range_t;
typedef struct fort_token_s fort_token_t;

#define FORT_CELL_TYPE(X) \
	X(FORT_NULL) \
	X(FORT_INTEGER) \
	X(FORT_REAL) \
	X(FORT_STRING) \
	X(FORT_XT) \
	X(FORT_OBJ)

BK_ENUM(fort_cell_type_t, FORT_CELL_TYPE)

#define FORT_ERR(X) \
	X(FORT_OK) \
	X(FORT_YIELD) \
	X(FORT_ERR_UNDERFLOW) \
	X(FORT_ERR_OVERFLOW) \
	X(FORT_ERR_OOM) \
	X(FORT_ERR_IO) \
	X(FORT_ERR_TYPE) \
	X(FORT_ERR_COMPILE_ONLY) \
	X(FORT_ERR_NOT_FOUND)

BK_ENUM(fort_err_t, FORT_ERR)

union fort_cell_u
{
	fort_int_t integer;
	fort_real_t real;
	void* ref;
};

struct fort_location_s
{
	uint16_t line;
	uint16_t column;
};

struct fort_loc_range_s
{
	fort_location_t start;
	fort_location_t end;
};

struct fort_string_ref_s
{
	const char* ptr;
	fort_int_t length;
};

struct fort_token_s
{
	fort_string_ref_t lexeme;
	fort_loc_range_t location;
};

struct fort_config_s
{
	struct bk_allocator_s* allocator;
	struct bk_file_s* output;
};

typedef fort_err_t(*fort_native_fn_t)(fort_t* fort, const fort_word_t* word);

FORT_DECL fort_err_t
fort_create(const fort_config_t* config, fort_t** fortp);

FORT_DECL void
fort_destroy(fort_t* fort);

FORT_DECL void
fort_reset(fort_t* fort);

/** @defgroup stack
 *  @{
 */

FORT_DECL fort_err_t
fort_push_integer(fort_t* fort, fort_int_t value);

FORT_DECL fort_err_t
fort_push_real(fort_t* fort, fort_real_t value);

FORT_DECL fort_err_t
fort_push_string(fort_t* fort, fort_string_ref_t string);

FORT_DECL fort_err_t
fort_pop(fort_t* fort, fort_cell_type_t* type, fort_cell_t* value);

FORT_DECL fort_err_t
fort_peek(
	fort_t* fort, fort_int_t index, fort_cell_type_t* type, fort_cell_t* value
);

FORT_DECL fort_int_t
fort_stack_size(fort_t* fort);

/** @} */

/** @defgroup scan
 *  @{
 */

FORT_DECL fort_err_t
fort_next_token(fort_t* fort, fort_token_t* token);

FORT_DECL fort_err_t
fort_next_char(fort_t* fort, char* ch);

/** @} */

/** @defgroup interpret
 *  @{
 */

FORT_DECL fort_err_t
fort_interpret(fort_t* fort, struct bk_file_s* in);

FORT_DECL fort_int_t
fort_is_interpreting(fort_t* fort);

/** @} */

#endif
