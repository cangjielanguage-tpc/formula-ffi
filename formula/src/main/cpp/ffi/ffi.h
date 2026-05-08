#ifndef FORMULA_FFI_H
#define FORMULA_FFI_H

#include <string>


extern "C" {
    int32_t formula_string_find(char* dStr, char* sStr);
    char* formula_tag(char* dStr);
    char* formula_xingTag(char* dStr);
}

#endif