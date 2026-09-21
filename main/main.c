#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_partition.h"

static const char *TAG = "LLM_INFERENCE";

// 1. Embedded tokenizer symbols
extern const uint8_t tok512_bin_start[] asm("_binary_tok512_bin_start");
extern const uint8_t tok512_bin_end[]   asm("_binary_tok512_bin_end");

// 2. Declare the external math engine from runq.c
extern void start_llm(char* prompt, void* model_weights_ptr, void* vocab_ptr);

// 3. Global pointers
esp_partition_mmap_handle_t mmap_handle;
const void *model_weights_ptr = NULL;

void llm_inference_task(void *pvParameters) {
    ESP_LOGI(TAG, "Inference Engine started on Core %d", xPortGetCoreID());
    char prompt_buffer[256];

    while (1) {
        printf("\n\n>>> Enter prompt: ");
        fflush(stdout);

        int idx = 0;
        memset(prompt_buffer, 0, sizeof(prompt_buffer));

        // Read UART input blockingly
        while (idx < sizeof(prompt_buffer) - 1) {
            int c = fgetc(stdin);
            if (c == '\n' || c == '\r') {
                if (idx > 0) break;
            } else if (c != EOF) {
                prompt_buffer[idx++] = (char)c;
                putchar(c);
                fflush(stdout);
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        printf("\n[Processing: '%s']\n", prompt_buffer);

        // 4. HAND CONTROL TO runq.c
        start_llm(prompt_buffer, (void*)model_weights_ptr, (void*)tok512_bin_start);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Initializing hardware...");

    const esp_partition_t *model_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "model"
    );

    if (model_partition == NULL) {
        ESP_LOGE(TAG, "Model partition not found!");
        return;
    }

    esp_err_t err = esp_partition_mmap(
        model_partition, 0, model_partition->size, 
        ESP_PARTITION_MMAP_DATA, &model_weights_ptr, &mmap_handle
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Memory mapping failed!");
        return;
    }
    
    ESP_LOGI(TAG, "Flash memory mapped successfully.");

    // Launch task on Core 1
    xTaskCreatePinnedToCore(
        llm_inference_task, "llm_task", 8192, NULL, 5, NULL, 1
    );
}