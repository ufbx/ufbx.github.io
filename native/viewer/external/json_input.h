#ifndef JSON_INPUT_H_INCLUDED
#define JSON_INPUT_H_INCLUDED

#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable: 4200)
#endif

#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunused-function"
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct jsi_dialect {
	unsigned allow_trailing_comma : 1;
	unsigned allow_missing_comma : 1;
	unsigned allow_bare_keys : 1;
	unsigned allow_comments : 1;
	unsigned allow_unknown_escape : 1;
	unsigned allow_control_in_string : 1;
	unsigned allow_equals_as_colon : 1;
} jsi_dialect;

typedef struct jsi_error {
	const char *description;
	size_t line, column;
	size_t byte_offset;
} jsi_error;

typedef void *(*jsi_alloc_fn)(void *user, size_t size);
typedef void *(*jsi_realloc_fn)(void *user, void *ptr, size_t new_size, size_t old_size);
typedef void (*jsi_free_fn)(void *user, void *ptr, size_t size);

typedef struct jsi_allocator {
	jsi_alloc_fn alloc_fn;
	jsi_realloc_fn realloc_fn;
	jsi_free_fn free_fn;
	void *user;

	size_t memory_used;
	size_t memory_limit;
} jsi_allocator;

typedef struct jsi_args {

	// Output
	jsi_error error;
	size_t result_used;
	size_t end_offset;

	// Input
	void *result_buffer;
	size_t result_size;

	void *temp_buffer;
	size_t temp_size;

	jsi_allocator result_allocator;
	jsi_allocator temp_allocator;

	int nesting_limit;

	unsigned allow_trailing_data : 1;
	unsigned no_allocation : 1;
	unsigned implicit_root_object : 1;
	unsigned implicit_root_array : 1;
	unsigned store_integers_as_int64 : 1;
	jsi_dialect dialect;

} jsi_args;

typedef enum jsi_type {
	jsi_type_undefined,
	jsi_type_null,
	jsi_type_boolean,
	jsi_type_number,
	jsi_type_string,
	jsi_type_object,
	jsi_type_array,
} jsi_type;

typedef struct jsi_obj jsi_obj;
typedef struct jsi_arr jsi_arr;
typedef struct jsi_obj_map jsi_obj_map;

typedef enum jsi_flag {
	jsi_flag_integer = 0x0001,
	jsi_flag_multiline = 0x0002,
	jsi_flag_stored_as_int64 = 0x0004,
} jsi_flag;

typedef struct jsi_value {
	jsi_type type;
	uint16_t key_hash;
	uint16_t flags;
	union {
		bool boolean;
		double number;
		int64_t int64_storage;
		const char *string;
		jsi_obj *object;
		jsi_arr *array;
	};
} jsi_value;

typedef struct jsi_prop {
	const char *key;
	jsi_value value;
} jsi_prop;

struct jsi_obj {
	jsi_obj_map *map;

	size_t num_props;
	jsi_prop props[];
};

struct jsi_arr {
	size_t num_values;
	jsi_value values[];
};

typedef const void *(*jsi_refill_fn)(void *user, size_t *size);

jsi_value *jsi_parse_memory(const void *data, size_t size, jsi_args *args);
jsi_value *jsi_parse_string(const char *str, jsi_args *args);
jsi_value *jsi_parse_file(const char *filename, jsi_args *args);
jsi_value *jsi_parse_stream(jsi_refill_fn refill, void *user, jsi_args *args);
jsi_value *jsi_parse_stream_initial(jsi_refill_fn refill, void *user, const void *data, size_t size, jsi_args *args);

void jsi_free(jsi_value *value);

jsi_value *jsi_get_len(jsi_obj *obj, const char *key, size_t length);
static jsi_value *jsi_get(jsi_obj *obj, const char *key) {
	return jsi_get_len(obj, key, strlen(key));
}

static size_t jsi_length(const char *jsi_str) {
	return (size_t)((const uint32_t*)jsi_str)[-1];
}
static size_t jsi_equal_len(const char *jsi_str, const char *str, size_t length) {
	return (size_t)((const uint32_t*)jsi_str)[-1] == length && !memcmp(jsi_str, str, length);
}
static size_t jsi_equal(const char *jsi_str, const char *str) {
	size_t length = strlen(str);
	return (size_t)((const uint32_t*)jsi_str)[-1] == length && !memcmp(jsi_str, str, length);
}

int jsi_as_bool(jsi_value *val, bool def);
int jsi_as_int(jsi_value *val, int def);
int64_t jsi_as_int64(jsi_value *val, int64_t def);
double jsi_as_double(jsi_value *val, double def);
const char *jsi_as_str(jsi_value *val, const char *def);
jsi_arr *jsi_as_arr(jsi_value *val);
jsi_obj *jsi_as_obj(jsi_value *val);

static int jsi_get_bool(jsi_obj *obj, const char *name, bool def) { return jsi_as_bool(jsi_get(obj, name), def); }
static int jsi_get_int(jsi_obj *obj, const char *name, int def) { return jsi_as_int(jsi_get(obj, name), def); }
static int64_t jsi_get_int64(jsi_obj *obj, const char *name, int64_t def) { return jsi_as_int64(jsi_get(obj, name), def); }
static double jsi_get_double(jsi_obj *obj, const char *name, double def) { return jsi_as_double(jsi_get(obj, name), def); }
static const char *jsi_get_str(jsi_obj *obj, const char *name, const char *def) { return jsi_as_str(jsi_get(obj, name), def); }
static jsi_arr *jsi_get_arr(jsi_obj *obj, const char *name) { return jsi_as_arr(jsi_get(obj, name)); }
static jsi_obj *jsi_get_obj(jsi_obj *obj, const char *name) { return jsi_as_obj(jsi_get(obj, name)); }

#ifdef __cplusplus
	static jsi_prop *begin(jsi_obj *obj) { return obj->props; }
	static jsi_prop *end(jsi_obj *obj) { return obj->props + obj->num_props; }
	static jsi_value *begin(jsi_arr *arr) { return arr->values; }
	static jsi_value *end(jsi_arr *arr) { return arr->values + arr->num_values; }
#endif

#if defined(__cplusplus)
}
#endif

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif

#if defined(__clang__)
	#pragma clang diagnostic pop
#endif

#endif
