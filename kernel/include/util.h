#pragma once
#include <isa/cpu.h>

// gets pointer of struct of type `type` given struct element pointer `ptr` of element name `member`
#define CONTAINER_OF(ptr, type, member) ((type*)((void*)ptr - offsetof(type, member)))
