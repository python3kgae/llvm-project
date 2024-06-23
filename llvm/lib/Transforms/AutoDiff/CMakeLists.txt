#if(WIN32 OR CYGWIN)
#  set(LLVM_LINK_COMPONENTS Core Support)
#endif()

if ("${Enzyme_TABLEGEN_EXE}" STREQUAL "")
  set(Enzyme_TABLEGEN_EXE enzyme-tblgen)
endif()

get_target_property(TBL_LINKED_LIBS LLVMSupport INTERFACE_LINK_LIBRARIES)
if (NOT TBL_LINKED_LIBS)
else()
list(REMOVE_ITEM TBL_LINKED_LIBS "ZLIB::ZLIB")
set_property(TARGET LLVMSupport PROPERTY INTERFACE_LINK_LIBRARIES ${TBL_LINKED_LIBS})
endif()

function(enzyme_tablegen ofn)
  tablegen(Enzyme ${ARGV})
  set(TABLEGEN_OUTPUT ${TABLEGEN_OUTPUT} ${CMAKE_CURRENT_BINARY_DIR}/${ofn}
      PARENT_SCOPE)
endfunction()

set(LLVM_TARGET_DEFINITIONS InstructionDerivatives.td)
enzyme_tablegen(BinopDerivatives.inc -gen-binop-derivatives)
enzyme_tablegen(IntrinsicDerivatives.inc -gen-intr-derivatives)
enzyme_tablegen(CallDerivatives.inc -gen-call-derivatives)
enzyme_tablegen(InstructionDerivatives.inc -gen-inst-derivatives)
enzyme_tablegen(BlasDerivatives.inc -gen-blas-derivatives)
enzyme_tablegen(BlasAttributor.inc -update-blas-declarations)
enzyme_tablegen(BlasTA.inc -gen-blas-typeanalysis)
enzyme_tablegen(BlasDiffUse.inc -gen-blas-diffuseanalysis)
add_public_tablegen_target(BinopDerivativesIncGen)
add_public_tablegen_target(IntrinsicDerivativesIncGen)
add_public_tablegen_target(CallDerivativesIncGen)
add_public_tablegen_target(InstructionDerivativesIncGen)
add_public_tablegen_target(BlasDerivativesIncGen)
add_public_tablegen_target(BlasDeclarationsIncGen)
add_public_tablegen_target(BlasTAIncGen)
add_public_tablegen_target(BlasDiffUseIncGen)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(LLVM_LINK_COMPONENTS Demangle)

file(GLOB ENZYME_SRC CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.cpp"
)
list(REMOVE_ITEM ENZYME_SRC "eopt.cpp")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND ENZYME_SRC SCEV/ScalarEvolutionExpander.cpp)
list(APPEND ENZYME_SRC TypeAnalysis/TypeTree.cpp TypeAnalysis/TypeAnalysis.cpp TypeAnalysis/TypeAnalysisPrinter.cpp TypeAnalysis/RustDebugInfo.cpp)

# on windows `PLUGIN_TOOL` doesn't link against LLVM.dll
if ((WIN32 OR CYGWIN) AND LLVM_LINK_LLVM_DYLIB)
    add_llvm_library( LLVMEnzyme-${LLVM_VERSION_MAJOR}
        ${ENZYME_SRC}
        PARTIAL_SOURCES_INTENDED
        MODULE
        DEPENDS
        intrinsics_gen
	LINK_COMPONENTS
	LLVM
    )
endif()

#if (${ENZYME_STATIC_LIB})
    add_llvm_library( LLVMEnzymeStatic-${LLVM_VERSION_MAJOR}
        ${ENZYME_SRC}
        PARTIAL_SOURCES_INTENDED
        STATIC
        DEPENDS
        intrinsics_gen
    )
#endif()

if (${ENZYME_EXTERNAL_SHARED_LIB})
    add_library( Enzyme-${LLVM_VERSION_MAJOR}
        SHARED
        ${ENZYME_SRC}
    )
    add_dependencies(Enzyme-${LLVM_VERSION_MAJOR} intrinsics_gen)
    add_dependencies(Enzyme-${LLVM_VERSION_MAJOR} BinopDerivativesIncGen)
    add_dependencies(Enzyme-${LLVM_VERSION_MAJOR} IntrinsicDerivativesIncGen)
    add_dependencies(Enzyme-${LLVM_VERSION_MAJOR} CallDerivativesIncGen)
    add_dependencies(Enzyme-${LLVM_VERSION_MAJOR} InstructionDerivativesIncGen)
    add_dependencies(Enzyme-${LLVM_VERSION_MAJOR} BlasDerivativesIncGen)
    add_dependencies(Enzyme-${LLVM_VERSION_MAJOR} BlasDeclarationsIncGen)
    add_dependencies(Enzyme-${LLVM_VERSION_MAJOR} BlasTAIncGen)
    add_dependencies(Enzyme-${LLVM_VERSION_MAJOR} BlasDiffUseIncGen)
    target_link_libraries(Enzyme-${LLVM_VERSION_MAJOR} LLVM)
    install(TARGETS Enzyme-${LLVM_VERSION_MAJOR}
        EXPORT EnzymeTargets
        LIBRARY DESTINATION lib COMPONENT shlib
        PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/Enzyme"
        COMPONENT dev)
endif()
