/* Implementations for functions ubsan will try to call.
 *
 * TODO: currently these just kernel panic which will give the location
 * and error type but a more proper impl would also give info like the
 * faulting value. */

#include <isa/cpu.h>

void __ubsan_handle_add_overflow(void) {
    kpanic("UBSan: add overflow");
}

void __ubsan_handle_type_mismatch_v1(void) {
    kpanic("UBSan: type mismatch v1");
}

void __ubsan_handle_pointer_overflow(void) {
    kpanic("UBSan: pointer overflow");
}

void __ubsan_handle_shift_out_of_bounds(void) {
    kpanic("UBSan: shift out of bounds");
}

void __ubsan_handle_out_of_bounds(void) {
    kpanic("UBSan: out of bounds");
}

void __ubsan_handle_load_invalid_value(void) {
    kpanic("UBSan: load invalid value");
}

void __ubsan_handle_mul_overflow(void) {
    kpanic("UBSan: mul overflow");
}

void __ubsan_handle_divrem_overflow(void) {
    kpanic("UBSan: div/rem overflow");
}

void __ubsan_handle_sub_overflow(void) {
    kpanic("UBSan: sub overflow");
}

void __ubsan_handle_invalid_builtin(void) {
    kpanic("UBSan: invalid builtin");
}
