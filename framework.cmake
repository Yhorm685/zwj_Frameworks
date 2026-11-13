target_include_directories(${FW_TARGET} PRIVATE "${FW_ROOT}")
target_compile_features(${FW_TARGET} PRIVATE cxx_std_17)
set_target_properties(${FW_TARGET} PROPERTIES CXX_EXTENSIONS OFF LINKER_LANGUAGE CXX)
target_compile_options(${FW_TARGET} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
target_sources(${FW_TARGET} PRIVATE
    "${FW_ROOT}/Application/console.cpp"
    "${FW_ROOT}/Application/services.cpp"
    "${FW_ROOT}/BSP/board.cpp"
    "${FW_ROOT}/BSP/uart.cpp"
    "${FW_ROOT}/Common/logger.cpp")

target_sources(${FW_TARGET} PRIVATE
    "${FW_ROOT}/BSP/board_timer.cpp"
    "${FW_ROOT}/BSP/timer.cpp"
    "${FW_ROOT}/BSP/pwm.cpp")
