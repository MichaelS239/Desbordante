set(NAME dd.fastdd.structs)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE model/pli_shard.cpp
            trees/minimize_tree.cpp
            trees/node.cpp
            trees/translating_minimize_tree.cpp
            trees/translating_tree_search.cpp
            util/cross_isn_builder.cpp
            util/diff_set_builder.cpp
            util/differential_function_builder.cpp
            util/evidence_inverter.cpp
            util/hybrid_evidence_inverter.cpp
            util/isn_info.cpp
            util/min_max_dif_calculator.cpp
            util/single_isn_builder.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE spdlog::spdlog_header_only better-enums Boost::headers)

set(NAME dd.fastdd)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE fastdd.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::dd::fastdd::structs
            ${DESBORDANTE_PREFIX}::dd
            ${DESBORDANTE_PREFIX}::model::table
            ${DESBORDANTE_PREFIX}::model::types
            spdlog::spdlog_header_only
            ${DESBORDANTE_PREFIX}::algos
            better-enums
            Boost::headers
)

set(NAME dd.fastdd_run)
add_executable(${NAME})
set_target_properties(${NAME} PROPERTIES LINK_LIBRARIES_ONLY_TARGETS ON)
target_sources(${NAME} PRIVATE main.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::dd::fastdd spdlog::spdlog_header_only
                    better-enums Boost::headers
)
