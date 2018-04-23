#ifndef FORT_H
#define FORT_H

#include <stddef.h>
#include <inttypes.h>
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
#define FORT_REAL_FMT "%f"
#define FORT_STR_TO_REAL strtod
#endif

#ifndef FORT_INT_TYPE
#define FORT_INT_TYPE int64_t
#define FORT_INT_FMT "%" PRId64
#define FORT_STR_TO_INT strtoll
#endif

struct bk_allocator_s;
struct bk_file_s;

typedef FORT_REAL_TYPE fort_real_t;
typedef FORT_INT_TYPE fort_int_t;
typedef struct fort_string_ref_s fort_string_ref_t;
typedef struct fort_s fort_t;
typedef struct fort_ctx_s fort_ctx_t;
typedef struct fort_word_s fort_word_t;
typedef struct fort_config_s fort_config_t;
typedef struct fort_ctx_config_s fort_ctx_config_t;
typedef struct fort_location_s fort_location_t;
typedef struct fort_loc_range_s fort_loc_range_t;
typedef struct fort_token_s fort_token_t;

#define FORT_CELL_TYPE(X) \
	X(FORT_NULL) \
	X(FORT_INTEGER) \
	X(FORT_REAL) \
	X(FORT_STRING) \
	X(FORT_XT) \
	X(FORT_TICK)

BK_ENUM(fort_cell_type_t, FORT_CELL_TYPE)

#define FORT_ERR(X) \
	X(FORT_OK) \
	X(FORT_YIELD) \
	X(FORT_SWITCH) \
	X(FORT_ERR_UNDERFLOW) \
	X(FORT_ERR_OVERFLOW) \
	X(FORT_ERR_OOM) \
	X(FORT_ERR_IO) \
	X(FORT_ERR_TYPE) \
	X(FORT_ERR_COMPILE_ONLY) \
	X(FORT_ERR_NOT_FOUND) \
	X(FORT_ERR_SYNTAX)

BK_ENUM(fort_err_t, FORT_ERR)

#define FORT_GC_OP(X) \
	X(FORT_GC_PAUSE) \
	X(FORT_GC_RESUME) \
	X(FORT_GC_STEP) \
	X(FORT_GC_COLLECT)

BK_ENUM(fort_gc_op_t, FORT_GC_OP)

#define FORT_STATE(X) \
	X(FORT_STATE_INTERPRETING) \
	X(FORT_STATE_COMPILING)

BK_ENUM(fort_state_t, FORT_STATE)

typedef enum fort_word_flag_e
{
	FORT_WORD_IMMEDIATE = 1 << 0,
	FORT_WORD_COMPILE_ONLY = 1 << 1
} fort_word_flag_t;

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
	size_t length;
};

struct fort_token_s
{
	fort_string_ref_t lexeme;
	fort_loc_range_t location;
};

struct fort_ctx_config_s
{
	struct bk_allocator_s* allocator;
};

struct fort_config_s
{
	struct bk_file_s* output;
};

typedef fort_err_t(*fort_native_fn_t)(fort_t* fort, fort_word_t* word);

/** @defgroup main
 *  @{
 */

FORT_DECL fort_err_t
fort_create_ctx(const fort_ctx_config_t* config, fort_ctx_t** ctxp);

FORT_DECL void
fort_destroy_ctx(fort_ctx_t* ctx);

FORT_DECL void
fort_reset_ctx(fort_ctx_t* ctx);

FORT_DECL fort_err_t
fort_create(fort_ctx_t* ctx, const fort_config_t* config, fort_t** fortp);

FORT_DECL void
fort_destroy(fort_t* fort);

FORT_DECL fort_err_t
fort_reset(fort_t* fort);

FORT_DECL fort_err_t
fort_load_builtins(fort_t* fort);

FORT_DECL fort_ctx_t*
fort_get_ctx(fort_t* fort);

FORT_DECL fort_state_t
fort_get_state(fort_t* fort);

FORT_DECL void
fort_set_state(fort_t* fort, fort_state_t state);

/** @} */

/** @defgroup stack
 *  @{
 */

FORT_DECL fort_err_t
fort_push_null(fort_t* fort);

FORT_DECL fort_err_t
fort_push_integer(fort_t* fort, fort_int_t value);

FORT_DECL fort_err_t
fort_push_real(fort_t* fort, fort_real_t value);

FORT_DECL fort_err_t
fort_push_string(fort_t* fort, fort_string_ref_t string);

FORT_DECL fort_err_t
fort_ndrop(fort_t* fort, fort_int_t count);

FORT_DECL fort_err_t
fort_pick(fort_t* fort, fort_int_t index);

FORT_DECL fort_err_t
fort_roll(fort_t* fort, fort_int_t n);

FORT_DECL fort_err_t
fort_move(fort_t* fort, fort_int_t src, fort_int_t dst);

FORT_DECL fort_err_t
fort_xmove(fort_t* src_fort, fort_t* dst_fort, fort_int_t index);

FORT_DECL fort_int_t
fort_get_stack_size(fort_t* fort);

FORT_DECL fort_err_t
fort_get_type(fort_t* fort, fort_int_t index, fort_cell_type_t* typep);

FORT_DECL fort_err_t
fort_as_string(fort_t* fort, fort_int_t index, fort_string_ref_t* value);

FORT_DECL fort_err_t
fort_as_integer(fort_t* fort, fort_int_t index, fort_int_t* value);

FORT_DECL fort_err_t
fort_as_real(fort_t* fort, fort_int_t index, fort_real_t* value);

FORT_DECL fort_err_t
fort_as_bool(fort_t* fort, fort_int_t index, fort_int_t* value);

/** @} */

/** @defgroup scan
 *  @{
 */

FORT_DECL fort_err_t
fort_next_token(fort_t* fort, fort_token_t* token);

FORT_DECL fort_err_t
fort_next_char(fort_t* fort, char* ch);

/** @} */

/** @defgroup exec
 *  @{
 */

FORT_DECL fort_err_t
fort_interpret(fort_t* fort, struct bk_file_s* in, fort_string_ref_t filename);

FORT_DECL fort_err_t
fort_interpret_string(fort_t* fort, fort_string_ref_t str, fort_string_ref_t filename);

FORT_DECL fort_err_t
fort_execute(fort_t* fort);

/** @} */

/** @defgroup dict
 *  @{
 */

FORT_DECL fort_err_t
fort_find(fort_t* fort, fort_string_ref_t name);

FORT_DECL fort_err_t
fort_create_word(
	fort_ctx_t* ctx, fort_string_ref_t name, fort_native_fn_t code,
	fort_int_t flags
);

FORT_DECL fort_err_t
fort_create_unnamed_word(fort_t* fort);

FORT_DECL fort_err_t
fort_get_word_code(fort_t* fort, fort_word_t* word, fort_native_fn_t* code);

FORT_DECL fort_err_t
fort_set_word_code(fort_t* fort, fort_word_t* word, fort_native_fn_t code);

FORT_DECL fort_err_t
fort_get_word_flags(fort_t* fort, fort_word_t* word);

FORT_DECL fort_err_t
fort_set_word_flags(fort_t* fort, fort_word_t* word);

FORT_DECL fort_err_t
fort_register_word(fort_t* fort, fort_word_t* word);

FORT_DECL fort_err_t
fort_get_word_data(fort_t* fort, fort_word_t* word, fort_int_t index);

FORT_DECL fort_err_t
fort_set_word_data(fort_t* fort, fort_word_t* word, fort_int_t index);

FORT_DECL fort_err_t
fort_push_word_data(fort_t* fort, fort_word_t* word);

FORT_DECL fort_err_t
fort_delete_word_data(fort_t* fort, fort_word_t* word, fort_int_t index);

FORT_DECL fort_err_t
fort_clear_word_data(fort_t* fort, fort_word_t* word);

FORT_DECL fort_err_t
fort_get_word_length(fort_t* fort, fort_word_t* word);

FORT_DECL fort_err_t
fort_get_word_name(fort_t* fort, fort_word_t* word);

FORT_DECL fort_err_t
fort_set_word_name(fort_t* fort, fort_word_t* word);

/** @} */

/** @defgroup serialization
 *  @{
 */

FORT_DECL fort_err_t
fort_save_image(fort_ctx_t* ctx, struct bk_file_s* output);

FORT_DECL fort_err_t
fort_load_image(fort_ctx_t* ctx, struct bk_file_s* input);

/** @} */

/** @defgroup gc
 *  @{
 */

FORT_DECL fort_err_t
fort_gc(fort_ctx_t* ctx, fort_gc_op_t op);

/** @} */

#endif
