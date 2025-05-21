#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"

#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/buzzer.h"
#include "lib/ws2812.h"
#include "ws2812.pio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdio.h>

// Definições de pinos e interfaces
#define I2C_PORT       i2c1
#define I2C_SDA        14
#define I2C_SCL        15
#define DISPLAY_ADDR   0x3C
#define ADC_X          26
#define ADC_Y          27
#define LED_VERDE      11
#define LED_VERMELHO   13
#define BUZZER_GPIO    21
#define BOTAO_BOOT     6

// Estrutura para dados do joystick
typedef struct {
    uint16_t x_pos;
    uint16_t y_pos;
} joystick_data_t;

// Estrutura para status do sistema
typedef struct {
    joystick_data_t data;
    bool alerta_ativo;
} status_t;

// Filas para comunicação entre tarefas
QueueHandle_t xQueueJoystickData;
QueueHandle_t xQueueStatus;

// Protótipos de tarefas e funções auxiliares
void vModoTask(void *params);
void vJoystickTask(void *params);
void vDisplayTask(void *params);
void vLedRGBTask(void *params);
void vBuzzerTask(void *params);
void vMatrizLEDTask(void *params);
void gpio_irq_handler(uint gpio, uint32_t events);

// Função principal
int main() {
    gpio_init(BOTAO_BOOT);
    gpio_set_dir(BOTAO_BOOT, GPIO_IN);
    gpio_pull_up(BOTAO_BOOT);
    gpio_set_irq_enabled_with_callback(BOTAO_BOOT, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();

    xQueueJoystickData = xQueueCreate(5, sizeof(joystick_data_t));
    xQueueStatus = xQueueCreate(5, sizeof(status_t));

    xTaskCreate(vJoystickTask, "Joystick", 256, NULL, 1, NULL);
    xTaskCreate(vModoTask, "Modo", 256, NULL, 1, NULL);
    xTaskCreate(vDisplayTask, "Display", 512, NULL, 1, NULL);
    xTaskCreate(vLedRGBTask, "LED RGB", 256, NULL, 1, NULL);
    xTaskCreate(vBuzzerTask, "Buzzer", 256, NULL, 1, NULL);
    xTaskCreate(vMatrizLEDTask, "Matriz", 256, NULL, 1, NULL);

    vTaskStartScheduler();
    panic_unsupported();
}

// Tarefa de decisão de alerta
void vModoTask(void *params) {
    status_t status_atual;
    joystick_data_t joydata;

    while (1) {
        if (xQueueReceive(xQueueJoystickData, &joydata, portMAX_DELAY)) {
            bool alerta = (joydata.y_pos >= 2866 || joydata.x_pos >= 3276);
            status_atual.data = joydata;
            status_atual.alerta_ativo = alerta;

            xQueueSend(xQueueStatus, &status_atual, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

// Leitura do joystick via ADC
void vJoystickTask(void *params) {
    adc_init();
    adc_gpio_init(ADC_Y);
    adc_gpio_init(ADC_X);

    joystick_data_t joydata;

    while (1) {
        adc_select_input(0); // Y
        joydata.y_pos = adc_read();

        adc_select_input(1); // X
        joydata.x_pos = adc_read();

        xQueueSend(xQueueJoystickData, &joydata, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Exibição no display OLED
void vDisplayTask(void *params) {
    i2c_init(I2C_PORT, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, DISPLAY_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    status_t status_atual;
    char agua_str[20], chuva_str[20];

    while (1) {
        if (xQueueReceive(xQueueStatus, &status_atual, portMAX_DELAY)) {
            uint pct_agua = (status_atual.data.y_pos * 100) / 4095;
            uint pct_chuva = (status_atual.data.x_pos * 100) / 4095;

            sprintf(agua_str, "Agua: %d %%", pct_agua);
            sprintf(chuva_str, "Chuva: %d %%", pct_chuva);

            ssd1306_fill(&ssd, false);
            ssd1306_rect(&ssd, 3, 3, 122, 60, true, false);
            ssd1306_line(&ssd, 3, 20, 122, 20, true);
            ssd1306_line(&ssd, 3, 40, 122, 40, true);

            ssd1306_draw_string(&ssd, "  MONITORADOR", 5, 5);
            ssd1306_draw_string(&ssd,
                status_atual.alerta_ativo ? "  Enchente!" : "Estado Normal", 15, 26);
            ssd1306_draw_string(&ssd, chuva_str, 5, 44);
            ssd1306_draw_string(&ssd, agua_str, 5, 54);
            ssd1306_send_data(&ssd);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Controle do LED RGB via PWM
void vLedRGBTask(void *params) {
    gpio_set_function(LED_VERMELHO, GPIO_FUNC_PWM);
    gpio_set_function(LED_VERDE, GPIO_FUNC_PWM);

    uint slice_red = pwm_gpio_to_slice_num(LED_VERMELHO);
    uint slice_green = pwm_gpio_to_slice_num(LED_VERDE);

    pwm_set_wrap(slice_red, 100);
    pwm_set_wrap(slice_green, 100);
    pwm_set_enabled(slice_red, true);
    pwm_set_enabled(slice_green, true);

    status_t status_atual;

    while (1) {
        if (xQueueReceive(xQueueStatus, &status_atual, portMAX_DELAY)) {
            int nv_x = (status_atual.data.x_pos * 100) / 4095;
            int nv_y = (status_atual.data.y_pos * 100) / 4095;
            int alerta = (nv_x > nv_y) ? nv_x : nv_y;

            if (alerta < 60) {
                pwm_set_chan_level(slice_green, PWM_CHAN_B, 100);
                pwm_set_chan_level(slice_red, PWM_CHAN_B, 0);
            } else {
                int progresso = alerta - 60;
                progresso = (progresso > 20) ? 20 : progresso;

                int red_val = (progresso * 100) / 20;
                int green_val = 100 - red_val;

                pwm_set_chan_level(slice_red, PWM_CHAN_B, red_val);
                pwm_set_chan_level(slice_green, PWM_CHAN_B, green_val);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Tarefa para buzzer
void vBuzzerTask(void *params) {
    buzzer_init(BUZZER_GPIO);
    status_t status_atual;
    TickType_t wake_time = xTaskGetTickCount();

    while (1) {
        if (xQueueReceive(xQueueStatus, &status_atual, portMAX_DELAY)) {
            if (status_atual.alerta_ativo) {
                bool agua = status_atual.data.y_pos >= 2866;
                bool chuva = status_atual.data.x_pos >= 3276;
                buzzer_control(true, agua, chuva);
            } else {
                buzzer_control(false, false, false);
            }
        }
        buzzer_loop();
        vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(10));
    }
}

// Controle da matriz WS2812
void vMatrizLEDTask(void *params) {
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &pio_matrix_program);
    pio_matrix_program_init(pio, sm, offset, WS2812_PIN);
    clear_matrix(pio, sm);

    status_t status_atual;
    TickType_t wake;

    while (1) {
        if (xQueueReceive(xQueueStatus, &status_atual, portMAX_DELAY)) {
            wake = xTaskGetTickCount();
            if (status_atual.alerta_ativo) {
                set_pattern(pio, sm, 1, "vermelho");
                vTaskDelayUntil(&wake, pdMS_TO_TICKS(250));
            } else {
                set_pattern(pio, sm, 0, "azul");
            }
        }
    }
}

// Ativa modo BOOTSEL
void gpio_irq_handler(uint gpio, uint32_t events) {
    reset_usb_boot(0, 0);
}
