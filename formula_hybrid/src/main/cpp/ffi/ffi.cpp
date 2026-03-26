//
// Created on 2026/3/16.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".
#include "ffi.h"
#include <unistd.h>
#include <hilog/log.h>

#define TAG_NOTAG 0
#define TAG_XING 1
#define TAG_NOXING 2

std::vector<std::string> alignmentXClass = {"aligned*", "align*", "flalign*", "gather*", "multline*", "eqnarray*"};
std::vector<std::string> alignmentClass = {"aligned", "align", "flalign", "gather", "multline", "eqnarray"};

int32_t formula_string_find(char* dStr, char* sStr) {
    std::string fatherStr(dStr);
    
    std::string sonStr(sStr);
    std::string sonStr1(sonStr + "*");
    std::string sonStr2(sonStr);
    
    if (fatherStr.find(sonStr1) != std::string::npos) {
        return TAG_XING;
    } else if (fatherStr.find(sonStr2) != std::string::npos) {
        return TAG_NOXING;
    }
    return TAG_NOTAG;
}

char* formula_tag(char* dStr) {
    std::string fstr(dStr);

    bool isMultline = false;

    size_t findBeginPos = 0;
    size_t findEndPos = 0;

    for (auto& alignClass : alignmentClass) {
        size_t alignPos = 0;
        auto tempBeginStr = "begin{" + alignClass + "}";
        auto tempEndStr = "end{" + alignClass + "}";
        if ((alignPos = fstr.find(tempBeginStr, alignPos)) != std::string::npos && alignPos < fstr.length()) {
            findBeginPos = alignPos + tempBeginStr.length();
            if ((alignPos = fstr.find(tempEndStr, alignPos)) != std::string::npos && alignPos < fstr.length()) {
                findEndPos = alignPos;
                break;
            }
        }
    }

    int mulLineNum = 0;
    if(fstr.find("multline", 0) != std::string::npos) {
        isMultline = true;
        int mulPos = findBeginPos;
        // 记录findBeginPos到findEndPos之间有多少个“\\”
        while ((mulPos = fstr.find("\\\\", mulPos)) != std::string::npos && mulPos < findEndPos) {
            ++mulPos;
            ++mulLineNum;
            OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "FORMULA", "---- mulLineNum: %{public}d", mulLineNum);
        }
        mulLineNum += 1;
    }
    
    int tagNum = 0;
    size_t pos = findBeginPos;

    std::vector<size_t> tagPoss;
    
    size_t tagPos = 0;
    while ((tagPos = fstr.find("tag{", tagPos)) != std::string::npos && tagPos < fstr.length()) {
        size_t tagEndPos = fstr.find("}", tagPos);
        if (tagEndPos == std::string::npos) {
            break;
        }
        tagPoss.push_back(tagEndPos + 1);
        tagPos = tagEndPos + 1;
    }

    size_t notagPos = 0;
    while ((notagPos = fstr.find("notag", notagPos)) != std::string::npos && notagPos < fstr.length()) {
        size_t tagEndPos = fstr.find("g", notagPos);
        tagPoss.push_back(tagEndPos + 1);
        notagPos += 5;
    }

    std::sort(tagPoss.begin(), tagPoss.end());
    
    int lineNum = 0;
    while (pos >= findBeginPos && (pos = fstr.find('\n', pos)) != std::string::npos  && pos < findEndPos) {
        if (lineNum > 0 && pos > 0 && mulLineNum <= 0) {
            int tempLen = 0;
            while (fstr[pos - 1] == '\\' || isspace(fstr[pos - 1])) {
                --pos;
                ++tempLen;
            }
            
            // 遍历tagPoss,查看是否有在pos之前的tag,如果有则无需在‘\n’前插入\tag{tagNum}
            bool isInsertTag = true;
            if (std::find(tagPoss.begin(), tagPoss.end(), pos) != tagPoss.end()) {
                isInsertTag = false;
                tagPoss.erase(std::remove(tagPoss.begin(), tagPoss.end(), pos), tagPoss.end());
            }
            
            if (isInsertTag) {
                std::string tagStr = "\\tag{" + std::to_string(tagNum) + "}";
                for (auto it = tagPoss.begin(); it != tagPoss.end();++it) {
                    if (pos < *it) {
                        *it += tagStr.length();
                    }
                }
                fstr.insert(pos, tagStr);
                pos += tagStr.length() + tempLen;
                findEndPos += tagStr.length();
            } else {
                pos += tempLen + 1;
                continue;
            }
        }
        --mulLineNum;
        if (mulLineNum <= 0) {
            ++tagNum;
        }
        ++lineNum;        
        ++pos;
    }
    
    char* taggedStr = new char[fstr.length() + 1];
    strcpy(taggedStr, fstr.c_str());
    return taggedStr;
}

char* formula_xingTag(char* dStr) {    
    std::string fstr(dStr);
    // 把所有对齐类*换成X
    for (auto& alignClass : alignmentXClass) {
        size_t alignPos = 0;
        while ((alignPos = fstr.find(alignClass, alignPos)) != std::string::npos && alignPos < fstr.length()) {
            size_t starPos = alignPos + alignClass.length() - 1;
            fstr.replace(starPos, 1, "X");
            alignPos += 1;
        }
    }
    
    size_t findBeginPos = 0;
    size_t findEndPos = 0;

    for (auto& alignClass : alignmentClass) {
        size_t alignPos = 0;
        auto tempBeginStr = "begin{" + alignClass + "X}";
        auto tempEndStr = "end{" + alignClass + "X}";
        if ((alignPos = fstr.find(tempBeginStr, alignPos)) != std::string::npos && alignPos < fstr.length()) {
            findBeginPos = alignPos + tempBeginStr.length();
            if ((alignPos = fstr.find(tempEndStr, alignPos)) != std::string::npos && alignPos < fstr.length()) {
                findEndPos = alignPos;
                break;
            }
        }
    }

    int mulLineXNum = 0;
    if(fstr.find("multline*", 0) != std::string::npos) {
        int mulPos = findBeginPos;
        // 记录findBeginPos到findEndPos之间有多少个“//”
        while ((mulPos = fstr.find("//", mulPos)) != std::string::npos && mulPos < findEndPos) {
            ++mulLineXNum;
        }
    }

    size_t pos = 0;
    std::vector<size_t> tagPoss;

    size_t notagPos = 0;
    while ((notagPos = fstr.find("\\notag", notagPos)) != std::string::npos && notagPos < fstr.length()) {
        // 删除\notag
        fstr.erase(notagPos, 6);
        notagPos += 1;
    }

    size_t tagPos = 0;
    while ((tagPos = fstr.find("tag{", tagPos)) != std::string::npos && tagPos < fstr.length()) {
        size_t tagEndPos = fstr.find("}", tagPos);
        if (tagEndPos == std::string::npos) {
            break;
        }
        tagPoss.push_back(tagEndPos + 1);
        tagPos = tagEndPos + 1;
    }

    std::sort(tagPoss.begin(), tagPoss.end());

    int lineNum = 0;
    while (pos >= findBeginPos && (pos = fstr.find('\n', pos)) != std::string::npos && pos < findEndPos) {
        if (lineNum > 0 && pos > 0 && mulLineXNum <= 0) {
            int tempLen = 0;
            while (fstr[pos - 1] == '\\' || isspace(fstr[pos - 1])) {
                --pos;
                ++tempLen;
            }
            
            bool isInsertTag = true;
            if (std::find(tagPoss.begin(), tagPoss.end(), pos) != tagPoss.end()) {
                isInsertTag = false;
                tagPoss.erase(std::remove(tagPoss.begin(), tagPoss.end(), pos), tagPoss.end());
            }
            
            if (isInsertTag) {
                std::string tagStr = "\\notag";
                for (auto it = tagPoss.begin(); it != tagPoss.end(); ++it) {
                    if (pos < *it) {
                        *it += tagStr.length();
                    }
                }
                fstr.insert(pos, tagStr);
                pos += tagStr.length() + tempLen;
            } else {
                pos += tempLen + 1;
                continue;
            }
        }
        --mulLineXNum;
        ++lineNum;        
        ++pos;
    }
    
    char* taggedStr = new char[fstr.length() + 1];
    strcpy(taggedStr, fstr.c_str());
    return taggedStr;
}
