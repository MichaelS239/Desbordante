add_subdirectory(fastdd)
add_subdirectory(split)
add_subdirectory(dd_verifier)

set(NAME dd)
desbordante_add_lib(NAME INTERFACE)
target_sources(${NAME} INTERFACE FILE_SET HEADERS FILES dd.h BASE_DIRS "${PROJECT_SOURCE_DIR}/src")

set(NAME dd.fastdd_run)
add_executable(${NAME})
set_target_properties(${NAME} PROPERTIES LINK_LIBRARIES_ONLY_TARGETS ON)
target_sources(${NAME} PRIVATE main.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::dd::fastdd ${DESBORDANTE_PREFIX}::dd::split
                    spdlog::spdlog_header_only better-enums Boost::headers
)
