target_compile_features(${PROJECT_NAME} PRIVATE c_std_17 cxx_std_20)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CONSOLE
        _UNICODE
        UNICODE
        GLFW_DLL
        GLFW_INCLUDE_NONE
        GLM_FORCE_DEPTH_ZERO_TO_ONE=1
        IMGUI_DEFINE_MATH_OPERATORS
        MI_BUILD_SHARED
        MI_OVERRIDE
        _WIN32_WINNT=0x0A00 #Windows 10
    )
    # suppress errors
    target_compile_options(${PROJECT_NAME} PRIVATE
        /wd4100 # unreferenced formal parameter
        /wd4244 # conversion from 'int' to 'char'
        /wd4701 # potentially uninitialized local variable 'name' used
        /wd4189 # local variable is initialized but not referenced
        /wd4201 # declaration of 'pos' hides class member (imguizmo.quat)
        /wd4458 # declaration of 'v' hides class member (imguizmo.quat)
    )
    target_compile_options(${PROJECT_NAME} PRIVATE
        "/sdl"
        "/utf-8"
        "/Zi"
        "/permissive-"
        "/Zc:externC-"
        "/Zc:preprocessor"
        "/Zc:__cplusplus"
    )
    # msvc, increase stack [1MB->8MB] for Engine to prevent stack_overflow
    target_link_options(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:DEBUG>:/INCREMENTAL;/DEBUG:FASTLINK;/OPT:NOICF;/OPT:NOREF;/LTCG:off;/TIME;/STACK:8388608;>"
        "$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/LTCG:incremental;/OPT:ICF;/OPT:REF;/TIME;/STACK:8388608;>"
    )
endif()