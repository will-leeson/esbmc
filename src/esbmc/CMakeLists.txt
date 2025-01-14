if(ENABLE_OLD_FRONTEND)
  set(OLD_FRONTEND_TARGETS ansicfrontend cppfrontend)
endif()

if(ENABLE_SOLIDITY_FRONTEND)
  set(SOLIDITY_FRONTEND_TARGETS solidityfrontend)
endif()

if(ENABLE_GOTO_CONTRACTOR)
  set(GOTO_CONTRACTOR_TARGETS gotocontractor)
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/buildidobj.txt
  COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/buildidobj.py ${CMAKE_CURRENT_BINARY_DIR}/buildidobj.txt
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating ESBMC version ID"
  VERBATIM
)

set(BUILD_ID_OBJ_OUT ${CMAKE_CURRENT_BINARY_DIR}/buildidobj.txt)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/buildidobj.c
  COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/flail.py -o ${CMAKE_CURRENT_BINARY_DIR}/buildidobj.c ${BUILD_ID_OBJ_OUT}
  DEPENDS  ${BUILD_ID_OBJ_OUT} ${CMAKE_SOURCE_DIR}/scripts/flail.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Flailing build ID object"
  VERBATIM
)

add_executable (esbmc main.cpp esbmc_parseoptions.cpp bmc.cpp globals.cpp document_subgoals.cpp show_vcc.cpp options.cpp ${CMAKE_CURRENT_BINARY_DIR}/buildidobj.c)
target_include_directories(esbmc
    PRIVATE ${CMAKE_BINARY_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${Boost_INCLUDE_DIRS}
    PRIVATE ${TorchScatter_INCLUDE_DIR}
    PRIVATE ${TorchSparse_INCLUDE_DIR}
)

target_link_libraries(esbmc ${OLD_FRONTEND_TARGETS} ${SOLIDITY_FRONTEND_TARGETS} ${GOTO_CONTRACTOR_TARGETS} clangcfrontend TorchScatter::TorchScatter TorchSparse::TorchSparse prediction
  clangcppfrontend symex pointeranalysis langapi util_esbmc bigint 
  solvers clibs default_message gotoalgorithms curses ${Boost_LIBRARIES} util)

install(TARGETS esbmc DESTINATION bin)
set_property(TARGET esbmc PROPERTY CXX_STANDARD 14)
