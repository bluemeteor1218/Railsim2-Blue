file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/sources.txt LEGACY_SOURCES_REL)
set(LEGACY_SOURCES "")
foreach(f ${LEGACY_SOURCES_REL})
    list(APPEND LEGACY_SOURCES ${PROJECT_SOURCE_DIR}/${f})
endforeach()

add_executable(railsim2_legacy WIN32 ${LEGACY_SOURCES} ${PROJECT_SOURCE_DIR}/RailSim2.rc
    shim/d3d8_impl.cpp shim/d3dx8_impl.cpp shim/d3dx8_2d.cpp shim/null_backend.cpp shim/d3d12_backend.cpp)
target_include_directories(railsim2_legacy PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/shim
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR})
target_compile_definitions(railsim2_legacy PRIVATE WIN32 _WINDOWS _CRT_SECURE_NO_WARNINGS _ITERATOR_DEBUG_LEVEL=0)
target_compile_options(railsim2_legacy PRIVATE /EHsc /permissive /Zc:strictStrings- /source-charset:utf-8 /execution-charset:.932 /W1 /MP)
set_target_properties(railsim2_legacy PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
set_source_files_properties(${PROJECT_SOURCE_DIR}/RailSim2.rc PROPERTIES COMPILE_FLAGS "/I${CMAKE_CURRENT_SOURCE_DIR}/shim")
target_link_libraries(railsim2_legacy PRIVATE dinput8 dxguid dsound winmm imm32 comctl32 user32 gdi32 ole32 oleaut32 uuid)
set_source_files_properties(shim/d3d8_impl.cpp shim/d3dx8_impl.cpp shim/d3dx8_2d.cpp shim/null_backend.cpp shim/d3d12_backend.cpp PROPERTIES COMPILE_DEFINITIONS NOMINMAX)
