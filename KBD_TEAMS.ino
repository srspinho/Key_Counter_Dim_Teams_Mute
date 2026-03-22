/*********************************************************************
 * PROJETO: HID Remapper & System Monitor v2.0 (Edição de Ouro)
 * HARDWARE: RP2040 (Raspberry Pi Pico) + SSD1306 OLED (128x64 I2C)
 * RECURSOS: 
 * - AltGr + R -> \ (Barra invertida)
 * - AltGr + M -> Ctrl + Shift + M (Mute/Unmute Microsoft Teams)
 * - Contador de Teclas (Visual VCR OSD)
 * - Controle de Volume com Barra Larga e Filtro Anti-Disparo
 * - Smart Dimming (Redução de brilho após 30s de inatividade)
 *********************************************************************/

#include "usbh_helper.h"
#include <set>
#include <U8g2lib.h>
#include <Wire.h>

/* --- CONFIGURAÇÕES DE HARDWARE --- */
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Variáveis Globais (Volatile para segurança entre núcleos)
volatile uint32_t g_key_count = 0;
volatile uint8_t  g_volume = 50;
unsigned long last_volume_time = 0;
unsigned long last_activity_time = 0;
unsigned long teams_alert_timeout = 0; 
bool is_dimmed = false;
const uint32_t DIM_TIMEOUT = 30000; // 30 segundos para o Modo Noturno

// Controle de Teclado
std::set<uint8_t> keys_already_pressed;
volatile uint8_t dev_addr_keyboard = 0;
volatile uint8_t instance_keyboard = 0;

// Descritor HID (Teclado + Consumer Control para Volume)
uint8_t const desc_hid_report[] = {
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1)),
  TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(2))
};

Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_KEYBOARD, 2, false);

/* --- CORE 1: LÓGICA DE INTERFACE E USB HOST --- */

void update_display() {
  u8g2.clearBuffer();
  
  // 1. Moldura Principal Arredondada
  u8g2.drawRFrame(0, 0, 128, 64, 5);
  
  // 2. Título (Topo limpo)
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(12, 13, "SYSTEM MONITOR v2.0");
  u8g2.drawHLine(0, 18, 128); // Linha divisória

  // 3. Ícone da Lua Pulsante (Efeito de Respiração)
  if (is_dimmed) {
    // Usamos o millis() para criar um efeito que alterna a cada 1 segundo (1000ms)
    // Se o resto da divisão por 2000 for menor que 1000, desenha a lua.
    // Isso cria um efeito de "pisca-pisca" suave.
    if ((millis() % 2000) < 1200) { 
      u8g2.drawCircle(115, 28, 3);
      u8g2.setDrawColor(0);
      u8g2.drawCircle(113, 26, 3, U8G2_DRAW_ALL);
      u8g2.setDrawColor(1);
    }
  }

  // 4. Alerta do Microsoft Teams (Centralizado)
  if (millis() < teams_alert_timeout) {
    u8g2.drawRFrame(20, 28, 88, 18, 3);
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(32, 40, "TEAMS MUTE");
  } else {
    // 5. Contador Principal (VCR OSD)
    u8g2.setFont(u8g2_font_VCR_OSD_mn);
    char buf_keys[12];
    sprintf(buf_keys, "%lu", g_key_count);
    uint8_t x_pos_keys = (128 - u8g2.getStrWidth(buf_keys)) / 2;
    u8g2.drawStr(x_pos_keys, 46, buf_keys);
  }

  // 6. Barra de Volume (Base)
  u8g2.drawRFrame(8, 51, 112, 9, 3);
  int barWidth = map(g_volume, 0, 100, 0, 108);
  if (barWidth > 0) {
    u8g2.drawRBox(10, 53, barWidth, 5, 2); 
  }

  u8g2.sendBuffer();
}

void setup1() {
  rp2040_configure_pio_usb();
  Wire.setSDA(4); 
  Wire.setSCL(5);
  u8g2.begin();
  u8g2.setContrast(255); // Brilho inicial máximo
  USBHost.begin(1);
}

void loop1() {
  USBHost.task();
  uint32_t current_time = millis();
  static uint32_t last_check = 0;

  // Gerenciador de Atividade (Smart Dimming)
  if (g_key_count != last_check) {
    last_check = g_key_count;
    last_activity_time = current_time;
    if (is_dimmed) { 
      u8g2.setContrast(255); 
      is_dimmed = false; 
    }
  }

  if (!is_dimmed && (current_time - last_activity_time > DIM_TIMEOUT)) {
    u8g2.setContrast(20); // Reduz brilho após inatividade
    is_dimmed = true;
  }

  // Refresh do Display (~20 FPS)
  static uint32_t last_draw = 0;
  if (current_time - last_draw > 50) {
    update_display();
    last_draw = current_time;
  }
}

/* --- CORE 0: PROCESSAMENTO E REMAPEAMENTO --- */

void setup() {
  usb_hid.setReportCallback(NULL, set_report_callback);
  usb_hid.begin();
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // O Core 0 mantém a conexão USB ativa com o computador
}

// Lógica de contagem de teclas (Debounce por set)
void process_kbd_report(hid_keyboard_report_t const *report) {
  std::set<uint8_t> keys_in_this_report;
  for (uint8_t i = 0; i < 6; i++) {
    uint8_t key = report->keycode[i];
    if (key != 0) {
      keys_in_this_report.insert(key);
      if (keys_already_pressed.find(key) == keys_already_pressed.end()) {
        g_key_count++;
        keys_already_pressed.insert(key);
      }
    }
  }
  for (auto it = keys_already_pressed.begin(); it != keys_already_pressed.end();) {
    if (keys_in_this_report.find(*it) == keys_in_this_report.end()) {
      it = keys_already_pressed.erase(it);
    } else {
      ++it;
    }
  }
}

// Lógica de Remapeamento (AltGr + Tecla)
void remap_key(hid_keyboard_report_t const *original, hid_keyboard_report_t *remapped) {
  memcpy(remapped, original, sizeof(hid_keyboard_report_t));
  bool altGr = (original->modifier & KEYBOARD_MODIFIER_RIGHTALT);

  for (uint8_t i = 0; i < 6; i++) {
    uint8_t key = original->keycode[i];
    if (key == 0) continue;

    // ABNT2: AltGr + R -> \ (Barra Invertida)
    if (altGr && key == HID_KEY_R) {
      remapped->modifier &= ~KEYBOARD_MODIFIER_RIGHTALT;
      remapped->keycode[i] = 0x64; 
    }
    // TEAMS: AltGr + M -> Ctrl + Shift + M (Mute/Unmute)
    else if (altGr && key == HID_KEY_M) {
      remapped->modifier &= ~KEYBOARD_MODIFIER_RIGHTALT;
      remapped->modifier |= (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTSHIFT);
      teams_alert_timeout = millis() + 1500; // Ativa alerta visual no display
    }
  }
}

/* --- CALLBACKS USB HOST --- */

void set_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
  if (report_type == HID_REPORT_TYPE_OUTPUT && bufsize > 0 && dev_addr_keyboard != 0) {
    uint8_t led_state = buffer[0];
    tuh_hid_set_report(dev_addr_keyboard, instance_keyboard, 0, HID_REPORT_TYPE_OUTPUT, &led_state, 1);
    digitalWrite(LED_BUILTIN, (led_state & KEYBOARD_LED_CAPSLOCK) ? HIGH : LOW);
  }
}

extern "C" {
  void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
    tuh_hid_receive_report(dev_addr, instance);
    if (tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_KEYBOARD) {
      dev_addr_keyboard = dev_addr;
      instance_keyboard = instance;
    }
  }

  void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    // Processamento de Teclado
    if (len == sizeof(hid_keyboard_report_t)) {
      hid_keyboard_report_t const *kbd_report = (hid_keyboard_report_t const *)report;
      process_kbd_report(kbd_report);
      hid_keyboard_report_t remapped;
      remap_key(kbd_report, &remapped);
      if (usb_hid.ready()) usb_hid.sendReport(1, &remapped, sizeof(hid_keyboard_report_t));
    } 
    // Processamento de Volume (Encoder Multimídia)
    else if (len == 3 && report[0] == 0x03) {
      unsigned long current_time = millis();
      if (current_time - last_volume_time > 50) { // Filtro anti-disparo
        int temp_vol = (int)g_volume;
        if (report[1] == 0xE9) temp_vol += 2;
        else if (report[1] == 0xEA) temp_vol -= 2;
        
        if (temp_vol > 100) temp_vol = 100;
        if (temp_vol < 0) temp_vol = 0;
        g_volume = (uint8_t)temp_vol;
        last_volume_time = current_time;
      }
      uint8_t media_data[2] = {report[1], report[2]};
      if (usb_hid.ready()) usb_hid.sendReport(2, media_data, 2);
    }
    tuh_hid_receive_report(dev_addr, instance);
  }
}