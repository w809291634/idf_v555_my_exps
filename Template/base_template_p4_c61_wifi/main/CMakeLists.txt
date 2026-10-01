# Build description of the application component.
#
# The xiaozhi-esp32 sources are NOT copied into this project: only the files this
# project builds are referenced in place from the tree below. The files that had
# to be adapted live here:
#   src/main.c            - template entry point (hw_board_init + console)
#   src/app_xiaozhi.cc    - starts Application::GetInstance()
#   src/board_p4_audio.cc - audio-only board (no screen, no camera)
#   Kconfig.projbuild     - xiaozhi menus + the template console menu
#   idf_component.yml     - trimmed to the components this project uses
#
# $ENV{IDF_PATH} may use backslashes on Windows, and a backslash is an escape
# character inside a CMake string - normalise it before building paths from it.
file(TO_CMAKE_PATH "$ENV{IDF_PATH}" XIAOZHI_IDF_PATH)
set(XIAOZHI_DIR "${XIAOZHI_IDF_PATH}/examples/idf_v555_my_exps/Template/base_tem_p4_c61_wifi_xiaozhi/xiaozhi-esp32")
set(XZ_MAIN "${XIAOZHI_DIR}/main")
if(NOT EXISTS "${XZ_MAIN}/application.cc")
    message(FATAL_ERROR "xiaozhi-esp32 sources not found at ${XIAOZHI_DIR}")
endif()

# ---------------------------------------------------------------------------
# Board identity
#
# BOARD_TYPE / BOARD_NAME are reported to the server (WiFi board JSON, MCP
# serverInfo, HTTP user agent). BUILTIN_*_FONT and DEFAULT_EMOJI_COLLECTION pick
# what goes into the generated assets partition. Only this one board is built, so
# the upstream table of ~200 board types is not needed here.
# ---------------------------------------------------------------------------
set(BOARD_TYPE "esp32-p4-wifi6-touch-lcd")
set(BUILTIN_TEXT_FONT font_puhui_basic_30_4)
set(BUILTIN_ICON_FONT font_awesome_30_4)
set(DEFAULT_EMOJI_COLLECTION twemoji_64)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(SOURCES
    # Assistant core
    "${XZ_MAIN}/application.cc"
    "${XZ_MAIN}/assets.cc"
    "${XZ_MAIN}/device_state_machine.cc"
    "${XZ_MAIN}/mcp_server.cc"
    "${XZ_MAIN}/ota.cc"
    "${XZ_MAIN}/settings.cc"
    "${XZ_MAIN}/system_info.cc"
    # Protocols (both can be selected)
    "${XZ_MAIN}/protocols/protocol.cc"
    "${XZ_MAIN}/protocols/websocket_protocol.cc"
    "${XZ_MAIN}/protocols/mqtt_protocol.cc"
    # Audio: service, ogg demuxer, ES8311 + ES7210 codec, ESP-SR wake word
    "${XZ_MAIN}/audio/audio_codec.cc"
    "${XZ_MAIN}/audio/audio_debugger.cc"
    "${XZ_MAIN}/audio/audio_service.cc"
    "${XZ_MAIN}/audio/demuxer/ogg_demuxer.cc"
    "${XZ_MAIN}/audio/codecs/box_audio_codec.cc"
    # Board support: base class, WiFi board, BOOT button, backlight base class
    # (Backlight::SetBrightness is referenced by mcp_server.cc)
    "${XZ_MAIN}/boards/common/board.cc"
    "${XZ_MAIN}/boards/common/wifi_board.cc"
    "${XZ_MAIN}/boards/common/button.cc"
    "${XZ_MAIN}/boards/common/backlight.cc"
    # Display glue that cannot be dropped from this side: board.cc,
    # assets.cc and mcp_server.cc use dynamic_cast<> on these classes, which
    # keeps their typeinfo - and with it the whole LVGL display stack - in the
    # link even though the board owns no panel.
    "${XZ_MAIN}/display/display.cc"
    "${XZ_MAIN}/display/lcd_display.cc"
    "${XZ_MAIN}/display/oled_display.cc"
    "${XZ_MAIN}/display/emote_display.cc"
    "${XZ_MAIN}/display/lvgl_display/lvgl_display.cc"
    "${XZ_MAIN}/display/lvgl_display/lvgl_theme.cc"
    "${XZ_MAIN}/display/lvgl_display/lvgl_font.cc"
    "${XZ_MAIN}/display/lvgl_display/lvgl_image.cc"
    "${XZ_MAIN}/display/lvgl_display/emoji_collection.cc"
    "${XZ_MAIN}/display/lvgl_display/gif/lvgl_gif.cc"
    "${XZ_MAIN}/display/lvgl_display/gif/gifdec.c"
    "${XZ_MAIN}/display/lvgl_display/jpg/image_to_jpeg.cpp"
    "${XZ_MAIN}/display/lvgl_display/jpg/jpeg_to_image.c"
    # This project
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app_xiaozhi.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/board_p4_audio.cc"
)

# Wake word / AFE engine, one per chip family
if(CONFIG_IDF_TARGET_ESP32)
    list(APPEND SOURCES
        "${XZ_MAIN}/audio/engines/lite_audio_engine.cc"
        "${XZ_MAIN}/audio/wake_words/esp_wake_word.cc"
    )
else()
    list(APPEND SOURCES
        "${XZ_MAIN}/audio/engines/afe_audio_engine.cc"
        "${XZ_MAIN}/audio/wake_words/custom_wake_word.cc"
        "${XZ_MAIN}/audio/wake_words/wake_word_audio_cache.cc"
    )
endif()

set(INCLUDE_DIRS "${XZ_MAIN}" "${CMAKE_CURRENT_SOURCE_DIR}/include" "${PROJECT_DIR}" "${XZ_MAIN}/display" "${XZ_MAIN}/display/lvgl_display" "${XZ_MAIN}/display/lvgl_display/jpg" "${XZ_MAIN}/audio" "${XZ_MAIN}/audio/demuxer" "${XZ_MAIN}/protocols")
set(MAIN_PRIV_INCLUDE_DIRS "${XZ_MAIN}/boards/common")

# ---------------------------------------------------------------------------
# Language assets
# Language of the embedded prompts / sounds. This project ships zh-CN; en-US is
# always collected below as the fallback for files missing in the current
# language. Only these two directories are kept.
# ---------------------------------------------------------------------------
if(CONFIG_LANGUAGE_EN_US)
    set(LANG_DIR "en-US")
else()
    set(LANG_DIR "zh-CN")
endif()

# Collect current language audio files
file(GLOB LANG_SOUNDS ${XZ_MAIN}/assets/locales/${LANG_DIR}/*.ogg)

# Add en-US files as fallback for the ones missing in the current language
file(GLOB EN_US_SOUNDS ${XZ_MAIN}/assets/locales/en-US/*.ogg)
set(EXISTING_NAMES "")
foreach(SOUND_FILE ${LANG_SOUNDS})
    get_filename_component(FILENAME ${SOUND_FILE} NAME)
    list(APPEND EXISTING_NAMES ${FILENAME})
endforeach()
foreach(EN_SOUND ${EN_US_SOUNDS})
    get_filename_component(FILENAME ${EN_SOUND} NAME)
    if(NOT ${FILENAME} IN_LIST EXISTING_NAMES)
        list(APPEND LANG_SOUNDS ${EN_SOUND})
        message(STATUS "Using en-US fallback for missing audio: ${FILENAME}")
    endif()
endforeach()

file(GLOB COMMON_SOUNDS ${XZ_MAIN}/assets/common/*.ogg)

# ---------------------------------------------------------------------------
# Component
# ---------------------------------------------------------------------------
idf_component_register(SRCS ${SOURCES}
                    EMBED_FILES ${LANG_SOUNDS} ${COMMON_SOUNDS}
                    INCLUDE_DIRS ${INCLUDE_DIRS}
                    PRIV_INCLUDE_DIRS ${MAIN_PRIV_INCLUDE_DIRS}
                    WHOLE_ARCHIVE
                    PRIV_REQUIRES
                        esp_pm
                        esp_psram
                        esp_netif
                        esp_eth
                        esp_driver_gpio
                        esp_driver_uart
                        esp_driver_spi
                        esp_driver_i2c
                        esp_driver_i2s
                        esp_driver_jpeg
                        esp_driver_ppa
                        esp_app_format
                        app_update
                        spi_flash
                        console
                        efuse
                        bt
                        fatfs
                        board
                        board_config
                        apl_console
                        apl_console_cmd_system
                        apl_console_cmd_nvs
                        apl_utility
                        drv_led
                    )

# BOARD_TYPE / BOARD_NAME are consumed as string macros by xiaozhi's sources
# (wifi_board.cc, system_info.cc, mcp_server.cc, application.cc).
target_compile_definitions(${COMPONENT_LIB}
                    PRIVATE BOARD_TYPE=\"${BOARD_TYPE}\" BOARD_NAME=\"${BOARD_TYPE}\"
                    PRIVATE BUILTIN_TEXT_FONT=${BUILTIN_TEXT_FONT} BUILTIN_ICON_FONT=${BUILTIN_ICON_FONT}
                    )

# ---------------------------------------------------------------------------
# Generated assets partition
# ---------------------------------------------------------------------------
# Locate ESP-SR (wake word models) and xiaozhi-fonts
idf_build_get_property(build_components BUILD_COMPONENTS)
function(find_component_by_pattern PATTERN COMPONENT_VAR PATH_VAR)
    foreach(COMPONENT ${build_components})
        if(COMPONENT MATCHES "${PATTERN}")
            set(${COMPONENT_VAR} ${COMPONENT} PARENT_SCOPE)
            idf_component_get_property(COMPONENT_PATH ${COMPONENT} COMPONENT_DIR)
            set(${PATH_VAR} "${COMPONENT_PATH}" PARENT_SCOPE)
            break()
        endif()
    endforeach()
endfunction()

find_component_by_pattern("espressif__esp-sr" ESP_SR_COMPONENT ESP_SR_COMPONENT_PATH)
if(ESP_SR_COMPONENT_PATH)
    set(ESP_SR_MODEL_PATH "${ESP_SR_COMPONENT_PATH}/model")
endif()

find_component_by_pattern("xiaozhi-fonts" XIAOZHI_FONTS_COMPONENT XIAOZHI_FONTS_COMPONENT_PATH)
if(XIAOZHI_FONTS_COMPONENT_PATH)
    set(XIAOZHI_FONTS_PATH "${XIAOZHI_FONTS_COMPONENT_PATH}")
endif()

# Function to build the assets.bin from the committed assets
function(build_default_assets_bin)
    set(GENERATED_ASSETS_BIN "${CMAKE_BINARY_DIR}/generated_assets.bin")

    set(BUILD_ARGS
        "--sdkconfig" "${SDKCONFIG}"
        "--output" "${GENERATED_ASSETS_BIN}"
    )
    if(BUILTIN_TEXT_FONT)
        list(APPEND BUILD_ARGS "--builtin_text_font" "${BUILTIN_TEXT_FONT}")
    endif()
    if(DEFAULT_EMOJI_COLLECTION)
        list(APPEND BUILD_ARGS "--emoji_collection" "${DEFAULT_EMOJI_COLLECTION}")
    endif()
    list(APPEND BUILD_ARGS "--esp_sr_model_path" "${ESP_SR_MODEL_PATH}")
    list(APPEND BUILD_ARGS "--xiaozhi_fonts_path" "${XIAOZHI_FONTS_PATH}")

    add_custom_command(
        OUTPUT ${GENERATED_ASSETS_BIN}
        COMMAND python ${XIAOZHI_DIR}/scripts/build_default_assets.py ${BUILD_ARGS}
        DEPENDS
            ${SDKCONFIG}
            ${XIAOZHI_DIR}/scripts/build_default_assets.py
        COMMENT "Building default assets.bin based on configuration"
        VERBATIM
    )

    add_custom_target(generated_default_assets ALL
        DEPENDS ${GENERATED_ASSETS_BIN}
    )

    set(GENERATED_ASSETS_LOCAL_FILE ${GENERATED_ASSETS_BIN} PARENT_SCOPE)
    message(STATUS "Default assets build configured: ${GENERATED_ASSETS_BIN}")
endfunction()

partition_table_get_partition_info(size "--partition-name assets" "size")
partition_table_get_partition_info(offset "--partition-name assets" "offset")
if("${size}" AND "${offset}")
    if(CONFIG_FLASH_DEFAULT_ASSETS)
        build_default_assets_bin()
        esptool_py_flash_to_partition(flash "assets" "${GENERATED_ASSETS_LOCAL_FILE}")
        message(STATUS "Generated default assets flash configured: ${GENERATED_ASSETS_LOCAL_FILE} -> assets partition")
    elseif(CONFIG_FLASH_NONE_ASSETS)
        message(STATUS "Assets flashing disabled (FLASH_NONE_ASSETS)")
    else()
        message(WARNING "CONFIG_FLASH_CUSTOM_ASSETS is not supported by this project; no assets partition will be flashed")
    endif()
else()
    message(STATUS "Assets partition not found, using v1 partition table")
endif()
