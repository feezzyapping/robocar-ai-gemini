#include "driver/i2s.h"

// Pin I2S ke INMP441
#define I2S_WS 5   // L/R (word select)
#define I2S_SD 18   // Data input (SD)
#define I2S_SCK 19  // Serial clock (BCLK)

// Konfigurasi I2S
#define SAMPLE_RATE     16000
#define SAMPLE_BITS     I2S_BITS_PER_SAMPLE_16BIT
#define I2S_PORT        I2S_NUM_0
#define BUFFER_SIZE     1024

int16_t buffer[BUFFER_SIZE];

void setupI2S() {
  const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = SAMPLE_BITS,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_zero_dma_buffer(I2S_PORT);
}

void setup() {
  Serial.begin(115200);
  setupI2S();
  Serial.println("INMP441 Test - Serial Plotter");
}

void loop() {
  size_t bytesRead = 0;
  i2s_read(I2S_PORT, (void*)buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);

  int samples = bytesRead / sizeof(int16_t);
  for (int i = 0; i < samples; i++) {
    // Ubah nilai agar berada di rentang tampilan Plotter
    int32_t sample = buffer[i];
    Serial.println(sample);
  }
}
