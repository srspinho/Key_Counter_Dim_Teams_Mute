<p align="center">
  <img src="https://github.com/srspinho/Key_Counter_volume/blob/main/IMG_20260322_165428.jpg" width="400">
</p>

Este projeto transforma um Raspberry Pi Pico (RP2040) em um remapeador de teclado USB inteligente e monitor de sistema. Ele atua como um "man-in-the-middle", interceptando comandos de um teclado físico e remapeando teclas específicas enquanto exibe estatísticas em tempo real em um display OLED.

🚀 Funcionalidades
Remapeamento de Teclas: Converte a combinação AltGr + R para a tecla \ (barra invertida), facilitando o uso de teclados internacionais no padrão ABNT2.
Contador de Teclas: Um "odômetro" de digitação que conta quantos pressionamentos de tecla foram realizados.
Monitor de Volume: Exibe uma barra de volume visual e arredondada que responde aos comandos do Rotary Encoder do teclado.
Dual-Core Processing: - Core 0: Gerencia a pilha USB Device (comunicação com o PC).
Core 1: Gerencia o USB Host (leitura do teclado) e a atualização do Display OLED.
Filtro de Estabilidade: Implementação de debounce de software para evitar disparos acidentais no controle de volume.
🛠️ Hardware Necessário
Microcontrolador: Raspberry Pi Pico (ou qualquer placa RP2040).
Display: OLED SSD1306 128x64 (I2C).
Conexão USB: Cabo OTG ou adaptador para conectar o teclado físico ao Pico via PIO-USB.
📁 Estrutura de Conexão (Pinout)
Componente	Pino Pico (GPIO)
OLED SDA	GPIO 4
OLED SCL	GPIO 5
USB Host D+	GPIO 16 (ou conforme config)
USB Host D-	GPIO 17 (ou conforme config)
💻 Bibliotecas Utilizadas
U8g2 - Para renderização dos gráficos no OLED.
Adafruit_TinyUSB - Para as funções de USB Host e Device.
Pico-PIO-USB - Para habilitar USB Host via hardware PIO.
📝 Como usar
Configure o ambiente Arduino para o Raspberry Pi Pico.
Certifique-se de que a velocidade da CPU está em 120MHz ou 240MHz (necessário para USB Host).
Instale as bibliotecas listadas acima.
Carregue o código e conecte seu teclado na porta USB Host.


🚀 O que há de novo na v2.0 (Edição de Ouro)
Smart Dimming & Breathing Moon: Sistema de economia de energia e proteção contra burn-in que reduz o brilho após 30 segundos de inatividade, exibindo um ícone de lua pulsante.

Microsoft Teams Integration: Atalho direto via hardware (AltGr + M) para Mute/Unmute, com feedback visual instantâneo no OLED.

Ajuste de Ergonomia Visual: Layout otimizado para displays de 64 pixels de altura, utilizando fontes de alta visibilidade (VCR OSD) e molduras arredondadas para um acabamento "Premium".

Estabilidade de Clock: Configuração recomendada de 120MHz para garantir sincronia perfeita entre o PIO USB e o barramento I2C do display.

📝 Sugestão de Mensagem de Commit:
Se você gosta de manter um histórico limpo, aqui está uma sugestão:
feat: upgrade to v2.0 with 128x64 display, Teams mute macro, and breathing-moon dimming

💡 Uma última dica:
Como você mencionou que o projeto ficou ligado por longos períodos em teste, certifique-se de que o seu Pico e o Display estão bem fixos no case. Com a vibração da digitação no dia a dia, qualquer jumper solto pode causar aquele erro que vimos antes.
