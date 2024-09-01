/*
  DW Client
*/

#include <ArduinoBLE.h>
#include <M5Core2.h>

#define DW_SVC_UUID "12345678-1234-5678-1234-56789abcdef0"
#define DW_TEMP_CHAR_UUID "2a20"
#define DW_DIST_CHAR_UUID "2a0d"
#define DW_BUZZ_CTRL_CHAR_UUID "aa00"
#define NOT_CONNECTED_COLOR YELLOW
#define CONNECTED_COLOR DARKGREEN
#define BORDER_WIDTH 12
#define BUTTON_H 40
#define BUTTON_W 90
#define LCD_W 320
#define LCD_H 240
#define STATUS_1_X LCD_W / 4
#define STATUS_1_Y 75
#define STATUS_1_R 40
#define STATUS_2_X LCD_W / 4 * 3
#define STATUS_2_Y 75
#define STATUS_2_R 40
#define STATUS_1_BAD_COLOR RED
#define STATUS_1_GOOD_COLOR GREEN
#define STATUS_1_UNKNOWN_COLOR LIGHTGREY
#define STATUS_2_BAD_COLOR RED
#define STATUS_2_GOOD_COLOR GREEN
#define STATUS_2_UNKNOWN_COLOR LIGHTGREY

#define STATUS_1_LOW_VAL 100.
#define STATUS_1_HIGH_VAL 120.
#define STATUS_2_LOW_VAL 40.
#define STATUS_2_HIGH_VAL 212.

// Defines the buttons. Colors in format {bg, text, outline}
ButtonColors on_clrs = {RED, WHITE, WHITE};
ButtonColors off_clrs = {BLACK, WHITE, WHITE};
Button connect_b(20, 0, 0, 0, false, "connect", off_clrs, on_clrs, BC_DATUM);
Button buzz_ctrl_b(20, 0, 0, 0, false, "buzz", off_clrs, on_clrs, BC_DATUM);

bool g_is_connected = false;
float g_dist = 0.;
float g_temp = 0.;
uint32_t g_s1_c = STATUS_1_UNKNOWN_COLOR;
uint32_t g_s2_c = STATUS_2_UNKNOWN_COLOR;
uint32_t g_buzz_ctrl = 0;
BLECharacteristic g_dist_chr;
BLECharacteristic g_temp_chr;
BLECharacteristic g_buzz_ctrl_chr;

void connectHandler(Event &e)
{
  Button &b = *e.button;
  if (b != connect_b)
    return;
  Serial.println("Connecting");
  if (b != M5.background)
  {
    // Toggles the button color between black and blue
    b.off.bg = (b.off.bg == BLACK) ? BLUE : BLACK;
    b.draw();
  }
  bleConnect();
}
void buzzHandler(Event &e)
{
  Button &b = *e.button;
  if (b != buzz_ctrl_b)
    return;
  g_buzz_ctrl = (g_buzz_ctrl ? 0 : 1);
  Serial.printf("Turning buzz %d\n", g_buzz_ctrl);
  if (b != M5.background)
  {
    // Toggles the button color between black and blue
    b.off.bg = (b.off.bg == BLACK) ? BLUE : BLACK;
    b.draw();
  }
  g_buzz_ctrl_chr.writeValue(g_buzz_ctrl);
}

void setup()
{
  // Setup serial output
  Serial.begin(9600);
  while (!Serial)
    ;

  // Setup bluetooth
  if (!BLE.begin())
  {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    while (1)
      ;
  }

  Serial.println("Bluetooth® Low Energy Central - Peripheral Explorer");

  // start scanning for peripherals
  BLE.scan();
  M5.begin();
  M5.lcd.setTextColor(BLACK, WHITE);
  M5.Buttons.addHandler(connectHandler, E_TAP);
  M5.Buttons.addHandler(buzzHandler, E_TAP);
  drawScreen();
}

void drawScreen()
{
  Serial.printf("Sc %d %d\n", M5.lcd.width(), M5.lcd.height());
  M5.lcd.fillRect(0, 0, M5.lcd.width(), M5.lcd.height(), g_is_connected ? CONNECTED_COLOR : NOT_CONNECTED_COLOR);
  M5.lcd.fillRect(BORDER_WIDTH, BORDER_WIDTH, M5.lcd.width() - BORDER_WIDTH * 2, M5.lcd.height() - BORDER_WIDTH * 2, WHITE);
  doButtons();
  doStatus(g_s1_c, g_s2_c, g_dist, g_temp);
}

void doStatus(uint32_t s1_c, uint32_t s2_c, float s1_v, float s2_v)
{
  static char s1_cv[10];
  static char s2_cv[10];
  sprintf(s1_cv, "%f", s1_v);
  sprintf(s2_cv, "%f", s2_v);
  M5.lcd.drawString("Distance", STATUS_1_X, STATUS_1_Y - STATUS_1_R - 2);
  M5.lcd.fillCircle(STATUS_1_X, STATUS_1_Y, STATUS_1_R, s1_c);
  M5.lcd.drawString(s1_cv, STATUS_1_X, STATUS_1_Y + STATUS_1_R + 20);
  M5.lcd.drawString("Temperature", STATUS_2_X, STATUS_2_Y - STATUS_2_R - 2);
  M5.lcd.fillCircle(STATUS_2_X, STATUS_2_Y, STATUS_2_R, s2_c);
  M5.lcd.drawString(s2_cv, STATUS_2_X, STATUS_2_Y + STATUS_2_R + 20);
}
void doButtons()
{
  int16_t qw = M5.Lcd.width() / 4;
  int16_t h = M5.Lcd.height();
  connect_b.set(qw - BUTTON_W / 2, h - (BORDER_WIDTH + BUTTON_H + 10), BUTTON_W, BUTTON_H);
  buzz_ctrl_b.set((qw * 3) - BUTTON_W / 2, h - (BORDER_WIDTH + BUTTON_H + 10), BUTTON_W, BUTTON_H);

  M5.Buttons.draw();
}
void get_data(const unsigned char data[], int length, uint32_t &v)
{
  v = 0;
  memcpy(&v, data, length);
}
void get_data(const unsigned char data[], int length, float &v)
{
  v = 0;
  memcpy(&v, data, length);
}
void updateValues()
{
  if (g_is_connected)
  { // check if the characteristic is readable
    if (g_dist_chr.canRead())
    {
      // read the characteristic value
      g_dist_chr.read();

      if (g_dist_chr.valueLength() > 0)
      {
        uint32_t v = 0;
        get_data(g_dist_chr.value(), g_dist_chr.valueLength(), v);
        g_dist = (float)v;
        if (v < STATUS_1_LOW_VAL)
        {
          g_s1_c = STATUS_1_BAD_COLOR;
        }
        else if (v > STATUS_1_HIGH_VAL)
        {
          g_s1_c = STATUS_1_BAD_COLOR;
        }
        else
        {
          g_s1_c = STATUS_1_GOOD_COLOR;
        }
      }
      else
      {
        Serial.printf("Cant read dist\n");
      }
      if (g_temp_chr.canRead())
      {
        g_temp_chr.read();

        if (g_temp_chr.valueLength() > 0)
        {
          float v = 0;
          get_data(g_temp_chr.value(), g_temp_chr.valueLength(), v);
          g_temp = (float)v;
          if (v < STATUS_2_LOW_VAL)
          {
            g_s2_c = STATUS_2_BAD_COLOR;
          }
          else if (v > STATUS_2_HIGH_VAL)
          {
            g_s2_c = STATUS_2_BAD_COLOR;
          }
          else
          {
            g_s2_c = STATUS_2_GOOD_COLOR;
          }
        }
      }
      else
      {
        Serial.printf("Cant read temp\n");
      }
      doStatus(g_s1_c, g_s2_c, g_dist, g_temp);
    }
  }
}
void bleConnect()
{
  // check if a peripheral has been discovered
  for (;;)
  {

    BLEDevice peripheral = BLE.available();

    if (peripheral)
    {
      // discovered a peripheral, print out address, local name, and advertised service
      Serial.print("Found ");
      Serial.print(peripheral.address());
      Serial.print(" '");
      Serial.print(peripheral.localName());
      Serial.print("' ");
      Serial.print(peripheral.advertisedServiceUuid());
      Serial.println();
      g_is_connected = false;

      if (peripheral.localName() == "Zephyr Peripheral Sample Long")
      {
        // stop scanning
        BLE.stopScan();
        if (peripheral.connect())
        {
          Serial.println("Connected");
          g_is_connected = true;
        }
        else
        {
          Serial.println("Failed to connect!");
        }
        // Get dw service
        bool r = peripheral.discoverService(DW_SVC_UUID);
        Serial.printf("svc discover %d\n", r);
        BLEService dw_svc = peripheral.service(DW_SVC_UUID);
        Serial.printf("Svc uuid %s\n", dw_svc.uuid());
        g_temp_chr = dw_svc.characteristic(DW_TEMP_CHAR_UUID);
        Serial.printf("temp uuid %s\n", g_temp_chr.uuid());
        g_dist_chr = dw_svc.characteristic(DW_DIST_CHAR_UUID);
        Serial.printf("dist uuid %s\n", g_dist_chr.uuid());
        g_buzz_ctrl_chr = dw_svc.characteristic(DW_BUZZ_CTRL_CHAR_UUID);
        Serial.printf("buzz ctrl uuid %s\n", g_buzz_ctrl_chr.uuid());
        break;
      }
    }
    else
    {
      break;
    }
  }
  drawScreen();
}

uint64_t app_s = 0;
void loop()
{
  app_s++;
  if ((app_s % 10000) == 0)
  {
    updateValues();
  }
  M5.update();
}
