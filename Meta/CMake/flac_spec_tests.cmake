include(utils)

if(INCLUDE_FLAC_SPEC_TESTS)
    if (CMAKE_PROJECT_NAME STREQUAL "SerenityOS")
        set(SOURCE_DIR "${SerenityOS_SOURCE_DIR}")
    else()
        set(SOURCE_DIR "${SERENITY_PROJECT_ROOT}")
    endif()
    set(FLAC_SPEC_TEST_GZ_URL https://github.com/ietf-wg-cellar/flac-test-files/archive/refs/heads/main.tar.gz)

    set(FLAC_TEST_PATH ${CMAKE_BINARY_DIR}/Tests/LibAudio/FLAC CACHE PATH "Location of FLAC tests")
    set(FLAC_SPEC_TEST_GZ_PATH ${FLAC_TEST_PATH}/flac-spec-testsuite.tar.gz)
    set(FLAC_SPEC_TEST_PATH ${FLAC_TEST_PATH}/SpecTests)

    if(NOT EXISTS ${FLAC_SPEC_TEST_GZ_PATH})
        message(STATUS "Downloading the IETF CELLAR FLAC testsuite from ${FLAC_SPEC_TEST_GZ_URL}...")
        download_file(${FLAC_SPEC_TEST_GZ_URL} ${FLAC_SPEC_TEST_GZ_PATH})
    endif()

    if(EXISTS ${FLAC_SPEC_TEST_GZ_PATH} AND NOT EXISTS ${FLAC_SPEC_TEST_PATH})
        file(MAKE_DIRECTORY ${FLAC_SPEC_TEST_PATH})
        message(STATUS "Extracting the FLAC testsuite from ${FLAC_SPEC_TEST_GZ_PATH}...")
        execute_process(COMMAND "${TAR_TOOL}" -xzf ${FLAC_SPEC_TEST_GZ_PATH} -C ${FLAC_TEST_PATH} RESULT_VARIABLE tar_result)
        if (NOT tar_result EQUAL 0)
            message(FATAL_ERROR "Failed to unzip ${FLAC_TEST_PATH} from ${FLAC_SPEC_TEST_GZ_PATH} with status ${tar_result}")
        endif()
        file(RENAME "${FLAC_TEST_PATH}/flac-test-files-main/subset" ${FLAC_SPEC_TEST_PATH})
    endif()
endif()
