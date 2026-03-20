cmake_minimum_required(VERSION 3.5)
project(latex)

include(CheckCXXCompilerFlag)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -w")

include(cmake/clone-repo.cmake)
clone_repo(https://gitcode.com/Cangjie-TPC/formula-ffi.git microtex-v0.0.1-fix)

# copy res dir

file(COPY latex/res DESTINATION .)

# file(WRITE "latex/src/config.h" "#ifndef CONFIG_H_INCLUDED\n#define CONFIG_H_INCLUDED\n\n// Flag for debug\n#if defined(_DEBUG) || defined(DEBUG) || defined(__DEBUG__) || !defined(NDEBUG)\n#   ifndef __DEBUG\n#       define __DEBUG\n#   endif\n#endif\n\n// Check platforms\n#if defined(__linux__)\n#   ifdef __ANDROID__\n#       define __OS_Android__\n#   elif defined __OHOS__\n#       define __OS_ohos__\n#   else\n#       define __OS_Linux__\n#   endif\n#elif defined(_WIN32)\n#   ifdef __OHOS__\n#       define __OS_ohos__\n#   else\n#       define __OS_Windows__\n#   endif\n#endif\n// Other platforms...\n\n// Flag for if compile samples\n#if defined(__OS_Linux__) || defined(__OS_Windows__) || defined(MEM_CHECK)\n#   define __USE_SAMPLES\n#endif\n\n// Disable log if not in debug mode\n#ifndef __DEBUG\n#   undef HAVE_LOG\n#endif\n\n#endif  // CONFIG_H_INCLUDED\n")

# modify formula.cpp
# set(COMMANDFILE "modify.sh")
# file(WRITE ${COMMANDFILE} "sed -n -i '1h;1!H;\${g;s/for (size_t j = 0; j < _row; j++).*auto it = _array\\[j\\].begin();/for (size_t j = 0; j < _row; j++) {\\n        size_t n_row_columns = _array[j].size();\\n        if (n_row_columns < col) {\\n        for (unsigned int i = 0; i < col - n_row_columns; i++)\\n            _array[j].push_back(nullptr);\\n        }\\n        auto it = _array[j].begin();/;p}' latex/src/core/formula.cpp")
# execute_process(COMMAND sh ${COMMANDFILE} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
# file(REMOVE ${COMMANDFILE})

# source files

file(GLOB_RECURSE SRC "latex/src/*.cpp" "ffi/*.cpp")

# check operating system

message(STATUS "We are working on ohos")
include_directories("latex/src" "latex" "latex/src/xml")

# compile options

add_compile_definitions(__OHOS__)
add_link_options(-static-libstdc++ -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/export.map)

add_library(latex SHARED ${SRC})

target_link_libraries(latex native_drawing)

#only in Release type use strip
if(CMAKE_BUILD_TYPE)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        add_custom_command(TARGET ${PROJECT_NAME}
                    POST_BUILD
                    COMMAND ${CMAKE_STRIP} -s "$<TARGET_FILE:${PROJECT_NAME}>"
                )
    endif()
endif()



