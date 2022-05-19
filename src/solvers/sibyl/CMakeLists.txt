find_library(GMP_LIB gmp)

add_library(solversibyl sibyl_conv.cpp)
target_include_directories(solversibyl
        PRIVATE ${Boost_INCLUDE_DIRS}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(solversibyl fmt::fmt ${GMP_LIB})

# Add to solver link
target_link_libraries(solvers INTERFACE solversibyl)

set(ESBMC_ENABLE_sibyl 1 PARENT_SCOPE)
set(ESBMC_AVAILABLE_SOLVERS "${ESBMC_AVAILABLE_SOLVERS} sibyl" PARENT_SCOPE)