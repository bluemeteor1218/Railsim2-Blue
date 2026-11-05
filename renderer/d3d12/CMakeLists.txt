add_executable(railsim2_d3d12_renderer WIN32
    main.cpp
)

target_compile_features(railsim2_d3d12_renderer PRIVATE cxx_std_17)
target_compile_definitions(railsim2_d3d12_renderer PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)
target_link_libraries(railsim2_d3d12_renderer PRIVATE
    d3d12
    dxgi
)

if(MSVC)
    target_compile_options(railsim2_d3d12_renderer PRIVATE /W4 /permissive- /Zc:__cplusplus)
endif()
