/*
 * Audio-only board class for the Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3
 * (ESP32-P4 + onboard ESP32-C6 for WiFi).
 *
 * The board file that ships with the referenced xiaozhi-esp32 tree also brings
 * up the MIPI-DSI panel, the GT911 touch controller and the camera. This project
 * is driven by voice and by the serial console, so those parts are not built at
 * all (see XIAOZHI_NO_UI in main/CMakeLists.txt) and the board keeps only the
 * audio codec (ES8311 output / ES7210 input through BoxAudioCodec) plus the BOOT
 * button.
 *
 * GetDisplay() is intentionally not overridden: Board::GetDisplay() already
 * returns a NoDisplay instance, so every display call made by Application is
 * turned into a log line.
 *
 * Pin assignment follows the upstream board's config.h.
 */
#include "wifi_board.h"
#include "application.h"
#include "button.h"
#include "codecs/box_audio_codec.h"

#include <driver/i2c_master.h>
#include <esp_log.h>

#define TAG "BoardP4Audio"

#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000
#define AUDIO_INPUT_REFERENCE    true

#define AUDIO_I2S_GPIO_MCLK      GPIO_NUM_13
#define AUDIO_I2S_GPIO_WS        GPIO_NUM_10
#define AUDIO_I2S_GPIO_BCLK      GPIO_NUM_12
#define AUDIO_I2S_GPIO_DIN       GPIO_NUM_11
#define AUDIO_I2S_GPIO_DOUT      GPIO_NUM_9

#define AUDIO_CODEC_PA_PIN       GPIO_NUM_53
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_7
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_8
#define AUDIO_CODEC_ES8311_ADDR  ES8311_CODEC_DEFAULT_ADDR
#define AUDIO_CODEC_ES7210_ADDR  ES7210_CODEC_DEFAULT_ADDR

#define BOOT_BUTTON_GPIO         GPIO_NUM_35

class P4AudioBoard : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    Button boot_button_;

    void InitializeCodecI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_1,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_));
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            // Before the first connection the BOOT button enters Wi-Fi config
            // mode instead of toggling the chat state.
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
    }

public:
    P4AudioBoard() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeCodecI2c();
        InitializeButtons();
    }

    virtual AudioCodec* GetAudioCodec() override {
        static BoxAudioCodec audio_codec(
            i2c_bus_,
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK,
            AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT,
            AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN,
            AUDIO_CODEC_ES8311_ADDR,
            AUDIO_CODEC_ES7210_ADDR,
            AUDIO_INPUT_REFERENCE);
        return &audio_codec;
    }
};

DECLARE_BOARD(P4AudioBoard);
