/* Modified version of rdmsr's TinyUBSan from https://github.com/rdmsr/tinyubsan/blob/master/tinyubsan.c.
 * This file is, instead of the mpl2.0, under the Unlicense License in the public domain. */

#include <stdarg.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <kprintf.h>
#include <isa/cpu.h>

#define TINYUBSAN_TRAP() kpanic("UBSan (see error above)")
#define TINYUBSAN_PRINT(...) klogf(LOG_ERROR, __VA_ARGS__)

struct tu_source_location {
  const char *file;
  uint32_t line;
  uint32_t column;
};

struct tu_type_descriptor {
  uint16_t kind;
  uint16_t info;
  char name[];
};

struct tu_overflow_data {
  struct tu_source_location location;
  struct tu_type_descriptor *type;
};

struct tu_shift_out_of_bounds_data {
  struct tu_source_location location;
  struct tu_type_descriptor *left_type;
  struct tu_type_descriptor *right_type;
};

struct tu_invalid_value_data {
  struct tu_source_location location;
  struct tu_type_descriptor *type;
};

struct tu_array_out_of_bounds_data {
  struct tu_source_location location;
  struct tu_type_descriptor *array_type;
  struct tu_type_descriptor *index_type;
};

struct tu_type_mismatch_v1_data {
  struct tu_source_location location;
  struct tu_type_descriptor *type;
  unsigned char log_alignment;
  unsigned char type_check_kind;
};

struct tu_negative_vla_data {
  struct tu_source_location location;
  struct tu_type_descriptor *type;
};

struct tu_nonnull_return_data {
  struct tu_source_location location;
};

struct tu_nonnull_arg_data {
  struct tu_source_location location;
};

struct tu_unreachable_data {
  struct tu_source_location location;
};

struct tu_invalid_builtin_data {
  struct tu_source_location location;
  unsigned char kind;
};

struct tu_float_cast_overflow_data {
  struct tu_source_location location;
  struct tu_type_descriptor *from_type;
  struct tu_type_descriptor *to_type;
};

struct tu_pointer_overflow_data {
  struct tu_source_location location;
  struct tu_type_descriptor *type;
};

struct tu_function_type_mismatch_data {
    struct tu_source_location location;
    struct tu_type_descriptor *type;
};

#ifdef __cplusplus
extern "C" {
#endif

static void print_location(const char *message,
                           const struct tu_source_location loc) {
  const char *file = loc.file ? loc.file : "<unknown>";
  TINYUBSAN_PRINT("tinyubsan: %s at %s:%u:%u\n", message, file, loc.line,
                  loc.column);
}

static void print_type(const char *label,
                       const struct tu_type_descriptor *type) {
  if (!type || !type->name[0]) {
    return;
  }
  TINYUBSAN_PRINT("tinyubsan:   %s type: %s\n", label, type->name);
}

static void report(const char *message, const struct tu_source_location loc,
                   const struct tu_type_descriptor *type) {
  print_location(message, loc);
  print_type("value", type);
  TINYUBSAN_TRAP();
}

void __ubsan_handle_add_overflow(struct tu_overflow_data *data) {
  report("addition overflow", data->location, data->type);
}
void __ubsan_handle_sub_overflow(struct tu_overflow_data *data) {
  report("subtraction overflow", data->location, data->type);
}
void __ubsan_handle_mul_overflow(struct tu_overflow_data *data) {
  report("multiplication overflow", data->location, data->type);
}
void __ubsan_handle_divrem_overflow(struct tu_overflow_data *data) {
  report("division overflow", data->location, data->type);
}
void __ubsan_handle_negate_overflow(struct tu_overflow_data *data) {
  report("negation overflow", data->location, data->type);
}
void __ubsan_handle_pointer_overflow(struct tu_overflow_data *data) {
  report("pointer overflow", data->location, data->type);
}
void __ubsan_handle_shift_out_of_bounds(
    struct tu_shift_out_of_bounds_data *data) {
  print_location("shift out of bounds", data->location);
  print_type("left", data->left_type);
  print_type("right", data->right_type);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_load_invalid_value(struct tu_invalid_value_data *data) {
  report("invalid load value", data->location, data->type);
}
void __ubsan_handle_out_of_bounds(struct tu_array_out_of_bounds_data *data) {
  print_location("array out of bounds", data->location);
  print_type("array", data->array_type);
  print_type("index", data->index_type);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_type_mismatch_v1(struct tu_type_mismatch_v1_data *data,
                                     uintptr_t ptr) {
  if (!ptr) {
    report("use of NULL pointer", data->location, data->type);
  } else if (ptr & (((uintptr_t)1 << data->log_alignment) - 1)) {
    report("use of misaligned pointer", data->location, data->type);
  } else {
    report("insufficient space for object", data->location, data->type);
  }
}
void __ubsan_handle_vla_bound_not_positive(struct tu_negative_vla_data *data) {
  report("VLA bound not positive", data->location, data->type);
}
void __ubsan_handle_nonnull_return(struct tu_nonnull_return_data *data) {
  report("non-null return is null", data->location, NULL);
}
void __ubsan_handle_nonnull_arg(struct tu_nonnull_arg_data *data) {
  report("non-null argument is null", data->location, NULL);
}
void __ubsan_handle_builtin_unreachable(struct tu_unreachable_data *data) {
  report("unreachable code reached", data->location, NULL);
}
void __ubsan_handle_invalid_builtin(struct tu_invalid_builtin_data *data) {
  report("invalid builtin", data->location, NULL);
}
void __ubsan_handle_function_type_mismatch(struct tu_function_type_mismatch_data *data) {
  report("function type mismatch", data->location, data->type);
}

void __ubsan_handle_add_overflow_abort(struct tu_overflow_data *data) {
  __ubsan_handle_add_overflow(data);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_sub_overflow_abort(struct tu_overflow_data *data) {
  __ubsan_handle_sub_overflow(data);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_mul_overflow_abort(struct tu_overflow_data *data) {
  __ubsan_handle_mul_overflow(data);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_divrem_overflow_abort(struct tu_overflow_data *data) {
  __ubsan_handle_divrem_overflow(data);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_negate_overflow_abort(struct tu_overflow_data *data) {
  __ubsan_handle_negate_overflow(data);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_pointer_overflow_abort(struct tu_overflow_data *data) {
  __ubsan_handle_pointer_overflow(data);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_shift_out_of_bounds_abort(
    struct tu_shift_out_of_bounds_data *data) {
  __ubsan_handle_shift_out_of_bounds(data);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_load_invalid_value_abort(
    struct tu_invalid_value_data *data) {
  __ubsan_handle_load_invalid_value(data);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_out_of_bounds_abort(
    struct tu_array_out_of_bounds_data *data) {
  __ubsan_handle_out_of_bounds(data);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_type_mismatch_v1_abort(
    struct tu_type_mismatch_v1_data *data, uintptr_t ptr) {
  __ubsan_handle_type_mismatch_v1(data, ptr);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_vla_bound_not_positive_abort(
    struct tu_negative_vla_data *data) {
  __ubsan_handle_vla_bound_not_positive(data);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_nonnull_return_abort(struct tu_nonnull_return_data *data) {
  __ubsan_handle_nonnull_return(data);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_nonnull_arg_abort(struct tu_nonnull_arg_data *data) {
  __ubsan_handle_nonnull_arg(data);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_builtin_unreachable_abort(
    struct tu_unreachable_data *data) {
  __ubsan_handle_builtin_unreachable(data);
  TINYUBSAN_TRAP();
}
void __ubsan_handle_invalid_builtin_abort(
    struct tu_invalid_builtin_data *data) {
  __ubsan_handle_invalid_builtin(data);
  TINYUBSAN_TRAP();
}

#ifdef __cplusplus
}
#endif
