# Checkout remote repository
macro(clone_repo  url tag)
    set(GIT_EXECUTABLE git)
    set(GIT_REPO ${url})
    set(GIT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/latex)
    set(LOCK_FILE ${CMAKE_CURRENT_SOURCE_DIR}/.latex.clone.lock)
    set(DONE_FILE ${GIT_DIR}/.clone-done)

    # Check if clone is fully completed (not just directory exists)
    if(NOT EXISTS "${DONE_FILE}")
        # Use file lock to prevent race condition when x86 and arm build concurrently
        file(LOCK ${LOCK_FILE} RESULT_VARIABLE LOCK_RESULT TIMEOUT 120)
        # Double-check after acquiring lock (another process may have cloned while waiting)
        if(NOT EXISTS "${DONE_FILE}")
            # Remove incomplete directory if exists (from previous failed clone)
            if(EXISTS "${GIT_DIR}")
                message(STATUS "Removing incomplete clone directory: ${GIT_DIR}")
                file(REMOVE_RECURSE "${GIT_DIR}")
            endif()
            
            message(STATUS "Cloning ${GIT_EXECUTABLE} clone ${GIT_REPO} ${GIT_DIR}")
            execute_process(
                COMMAND ${GIT_EXECUTABLE} clone ${GIT_REPO} ${GIT_DIR} -b ${tag} --depth=1
                RESULT_VARIABLE GIT_CLONE_RESULT
                OUTPUT_VARIABLE GIT_CLONE_OUTPUT
                ERROR_VARIABLE GIT_CLONE_ERROR
            )
            if(NOT GIT_CLONE_RESULT EQUAL 0)
                file(LOCK ${LOCK_FILE} RELEASE)
                message(FATAL_ERROR "Git clone failed (exit code ${GIT_CLONE_RESULT}): ${GIT_CLONE_ERROR}")
            endif()
            
            # Create done marker file to indicate successful clone
            file(TOUCH "${DONE_FILE}")
            message(STATUS "Git clone completed successfully, marker created: ${DONE_FILE}")
        else()
            message(STATUS "Repository already cloned at ${GIT_DIR} (by another process)")
        endif()
        file(LOCK ${LOCK_FILE} RELEASE)
    else()
        message(STATUS "Repository already cloned at ${GIT_DIR}")
    endif()
endmacro()


