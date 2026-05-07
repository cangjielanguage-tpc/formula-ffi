/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#include "latex.h"

#ifdef __OS_ohos__

#include <ctype.h>
#include <string>
#include <cstring>

using namespace std;
using namespace tex;

#define TEX_OK                      0
#define TEX_ERROR_SYNTAX           1
#define TEX_ERROR_INVALID_MATRIX   2
#define TEX_ERROR_INVALID_DELIM    3
#define TEX_ERROR_TEX              4
#define TEX_ERROR_CHEMFIG          5
#define TEX_ERROR_UNKNOWN          99

static void copyErrorMessage(char* dest, const std::string& src, int destSize) {
    if (destSize <= 0) return;
    strncpy(dest, src.c_str(), destSize - 1);
    dest[destSize - 1] = '\0';
}

static void logError(const char* message, int resultCode, const char* formula) {
}

#ifdef __cplusplus
extern "C" {
#endif

bool LaTeX_init(char *rootDir) {
    try {
        LaTeX::init(rootDir);
        return true;
    } catch (ex_tex& e) {
        return false;
    }
}

void LaTeX_release() {
    LaTeX::release();
}

TeXRender *LaTeX_parse(char *ltx, int width, float textSize, float lineSpace, uint32_t foreground) {
    wstring value;
    value = utf82wide(ltx);
    const wchar_t *wstr = value.c_str();
    try {
        TeXRender *r = LaTeX::parse(value, width, textSize, lineSpace, foreground);
        return r;
    } catch (exception& e) {
        return nullptr;
    }
}

TeXRender* LaTeX_parse_with_error(char *ltx, int width, float textSize, 
                                   float lineSpace, uint32_t foreground,
                                   int *resultCode, char *errorMsg, 
                                   int errorMsgSize) {
    if (resultCode == nullptr || errorMsg == nullptr || errorMsgSize <= 0) {
        return nullptr;
    }
    
    *resultCode = TEX_OK;
    errorMsg[0] = '\0';
    
    if (ltx == nullptr) {
        *resultCode = TEX_ERROR_UNKNOWN;
        strncpy(errorMsg, "Input formula is null", errorMsgSize - 1);
        errorMsg[errorMsgSize - 1] = '\0';
        return nullptr;
    }
    
    wstring value;
    try {
        value = utf82wide(ltx);
    } catch (exception& e) {
        *resultCode = TEX_ERROR_UNKNOWN;
        strncpy(errorMsg, "Invalid UTF-8 encoding", errorMsgSize - 1);
        errorMsg[errorMsgSize - 1] = '\0';
        return nullptr;
    }
    
    try {
        LaTeXParseResult parseResult = LaTeX::parseWithError(value, width, textSize, lineSpace, foreground);
        
        if (parseResult.success) {
            TeXRender* render = parseResult.render;
            parseResult.render = nullptr;
            return render;
        } else {
            string msg = parseResult.errorMessage;
            if (msg.find("chemfig") != string::npos || msg.find("Chemfig") != string::npos) {
                *resultCode = TEX_ERROR_CHEMFIG;
            } else if (msg.find("invalid matrix") != string::npos || msg.find("column") != string::npos) {
                *resultCode = TEX_ERROR_INVALID_MATRIX;
            } else if (msg.find("delimiter") != string::npos || msg.find("bracket") != string::npos) {
                *resultCode = TEX_ERROR_INVALID_DELIM;
            } else if (msg.find("unknown") != string::npos || msg.find("Undefined") != string::npos) {
                *resultCode = TEX_ERROR_TEX;
            } else {
                *resultCode = TEX_ERROR_SYNTAX;
            }
            copyErrorMessage(errorMsg, msg, errorMsgSize);
            logError(msg.c_str(), *resultCode, ltx);
            return nullptr;
        }
    } catch (exception& e) {
        *resultCode = TEX_ERROR_UNKNOWN;
        copyErrorMessage(errorMsg, e.what(), errorMsgSize);
        logError(e.what(), TEX_ERROR_UNKNOWN, ltx);
        return nullptr;
    }
}

void LaTeX_setDebug(bool debug) {
    LaTeX::setDebug(debug);
}

#ifdef __cplusplus
}
#endif

#endif  // __OS_ohos__
