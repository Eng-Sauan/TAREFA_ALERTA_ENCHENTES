# 🌊 Estação de Alerta de Enchente com Simulação via Joystick

Projeto desenvolvido para simular uma estação de monitoramento de enchentes utilizando a placa BitDogLab (RP2040) com sensores simulados por joystick analógico. O sistema utiliza display OLED, buzzer, LED RGB, matriz WS2812 e sistema de filas com FreeRTOS.

## 📦 Componentes Utilizados

- **Placa**: Raspberry Pi Pico (BitDogLab)
- **Display**: OLED SSD1306 (via I2C)
- **Joystick**: Simula sensor de nível de água e intensidade de chuva
- **LED RGB**: Alerta visual proporcional à intensidade
- **Matriz WS2812B**: Efeitos visuais de alerta
- **Buzzer**: Alarme sonoro
- **Botão**: GPIO para BOOTSEL (modo flash)

## ⚙️ Funcionalidades

- **Leitura contínua do joystick via ADC**.
- **Detecção de risco de enchente** com base em thresholds configuráveis.
- **Exibição no display OLED** das leituras e do estado atual.
- **Alerta sonoro com buzzer** em caso de enchente.
- **Alteração da cor e intensidade do LED RGB** conforme gravidade.
- **Animações na matriz WS2812** para alerta visual.
- **Organização em múltiplas `tasks` do FreeRTOS** com uso de `queues`.

## 🧠 Lógica de Funcionamento

- O joystick Y simula o **nível de água**.
- O joystick X simula a **intensidade da chuva**.
- Se a leitura ultrapassar os limites pré-definidos:
  - **Alerta é ativado**
  - Display mostra o aviso
  - Buzzer toca
  - LEDs mudam de cor
  - Matriz exibe alerta vermelho

## 📐 Mapas de Pinos

| Função         | GPIO |
|----------------|------|
| SDA (I2C)      | 14   |
| SCL (I2C)      | 15   |
| Joystick Y     | 26   |
| Joystick X     | 27   |
| LED Verde      | 11   |
| LED Vermelho   | 13   |
| Buzzer         | 21   |
| Botão BOOTSEL  | 6    |

## 🛠️ Organização do Código

O projeto é dividido em múltiplas tasks FreeRTOS:

- `vJoystickTask`: Leitura dos valores do joystick
- `vModoTask`: Determina se há alerta de enchente
- `vDisplayTask`: Atualiza o display OLED
- `vLedRGBTask`: Controla o LED RGB com PWM
- `vBuzzerTask`: Toca o alarme sonoro
- `vMatrizLEDTask`: Controla a matriz de LEDs WS2812B
- `gpio_irq_handler`: Habilita BOOTSEL pelo botão B

## 🧱 Bibliotecas Utilizadas

- `ssd1306.h`: Controle do display OLED
- `font.h`: Fontes para display
- `buzzer.h`: Controle do buzzer
- `ws2812.h`, `ws2812.pio.h`: Controle de matriz WS2812
- FreeRTOS: Gerenciamento de tarefas e filas

## 🚨 Condições de Alerta

- **Chuva ≥ 80% (3276/4095)** ou **Água ≥ 70% (2866/4095)** ativa o modo de emergência.
- Durante o alerta:
  - Display mostra “Enchente!”
  - LED RGB alterna cor com intensidade
  - Buzzer toca com variação
  - Matriz de LEDs exibe padrão vermelho

## 📎 Compilação

Este projeto é compilado com **SDK do Raspberry Pi Pico** + **FreeRTOS** usando **CMake**.

Certifique-se de ter:
- Ambiente com CMake configurado
- Pico SDK instalado
- FreeRTOS incluído no projeto

---

## 💡 Inspiração e Aplicação

Este projeto simula uma estação de monitoramento de desastres naturais em ambientes ribeirinhos, podendo ser usado como base didática para aplicações reais de IoT em cidades inteligentes.
