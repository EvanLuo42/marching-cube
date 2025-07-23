cmake_minimum_required(VERSION 3.31)
project(marching_cube)

set(CMAKE_CXX_STANDARD 20)

if (APPLE)
    set(Vulkan_LIBRARY $ENV{VULKAN_SDK}/lib/libvulkan.dylib)
endif ()

set(Vulkan_INCLUDE_DIR $ENV{VULKAN_SDK}/include)

add_executable(marching_cube Source/main.cpp
        Source/Resource/ShaderManager.cpp
        Source/Resource/ShaderManager.h
        Source/Render/RenderContext.cpp
        Source/Render/RenderContext.h
        Source/Render/Renderer.cpp
        Source/Render/Renderer.h
        Source/Render/Utils.h
        Source/Render/Utils.cpp
)

target_include_directories(marching_cube PRIVATE
        ${CMAKE_SOURCE_DIR}/External
        ${CMAKE_SOURCE_DIR}/External/glm
        ${CMAKE_SOURCE_DIR}/External/vulkan/include
        ${CMAKE_SOURCE_DIR}/External/glfw/include
        ${CMAKE_SOURCE_DIR}/External/volk
)

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/External/imgui")

target_sources(marching_cube PRIVATE
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(marching_cube PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

add_subdirectory(External/glfw)
target_link_libraries(marching_cube PRIVATE glfw)

find_package(Vulkan REQUIRED)
target_link_libraries(marching_cube PRIVATE Vulkan::Vulkan)

if (APPLE)
    target_link_libraries(marching_cube PRIVATE $ENV{VULKAN_SDK}/lib/libslang.dylib)
endif ()
