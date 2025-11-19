#include <IRremote.hpp>
#include <Arduino_FreeRTOS.h>

#define IR_RECEIVE_PIN  11
#define IR_SEND_PIN     3
#define TIMEOUT_2MIN    (120000 / portTICK_PERIOD_MS)

TaskHandle_t recvTaskHandle = NULL;
TaskHandle_t cmdTaskHandle = NULL;

#define NO_LED_FEEDBACK_CODE
#define SCAN_ADDRESS 0x00
#define SCAN_REPEAT 2
#define SCAN_DELAY_MS 3000

volatile bool continuousReadActive = false;
volatile TickType_t readEndTime = 0;
volatile bool startScanNEC = false;
TickType_t lastScanTime = 0;
const TickType_t scanInterval = SCAN_DELAY_MS / portTICK_PERIOD_MS;

void setup() {
  Serial.begin(115200);

  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH); // 輸出邏輯高電位 ≈ 5V

  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  IrSender.begin(IR_SEND_PIN, DISABLE_LED_FEEDBACK);



  Serial.println(F("FreeRTOS 雙工紅外線系統啟動"));
  printHelp();

  xTaskCreate(taskSerialCommand, "CMD", 256, NULL, 2, &cmdTaskHandle);
  xTaskCreate(taskIRReceiver,   "RECV", 256, NULL, 1, &recvTaskHandle);

  vTaskStartScheduler();
}

void loop() {}


void taskIRReceiver(void *pvParameters) {
  for (;;) {
    // 接收紅外線
    if (IrReceiver.decode()) {
      Serial.print(F("[RECV] "));
      IrReceiver.printIRResultShort(&Serial);
      Serial.println();
      IrReceiver.resume();
    }

    // 掃描控制（每 3 秒執行一次）
    if (startScanNEC && xTaskGetTickCount() - lastScanTime >= scanInterval) {
      static uint16_t scanCmd = 0x00;

      Serial.print(F("[SCAN] 發送 Command = 0x"));
      Serial.println(scanCmd, HEX);

      IrSender.sendNEC(SCAN_ADDRESS, scanCmd, SCAN_REPEAT);
      lastScanTime = xTaskGetTickCount();

      scanCmd++;
      if (scanCmd > 0xFF) {
        Serial.println(F("[SCAN] 掃描完成"));
        startScanNEC = false;
        scanCmd = 0;
      }
    }


    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void taskSerialCommand(void *pvParameters) {
  String cmd = "";
  for (;;) {
    if (Serial.available()) {
      cmd = Serial.readStringUntil('\n');
      cmd.trim();
      cmd.toLowerCase();
      handleCommand(cmd);
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void handleCommand(String cmd) {
  if (cmd == "read") {
    taskENTER_CRITICAL();
    continuousReadActive = true;
    readEndTime = xTaskGetTickCount() + TIMEOUT_2MIN;
    taskEXIT_CRITICAL();
    Serial.println(F("[CMD] 開始連續接收2分鐘..."));
  }
  else if (cmd == "help") {
    printHelp();
  }
  else if (cmd.startsWith("sendnec ")) {
    String param = cmd.substring(8);
    param.trim();
    int space1 = param.indexOf(' ');
    int space2 = param.lastIndexOf(' ');

    if (space1 == -1 || space1 == space2) {
      Serial.println(F("格式錯誤：sendnec <address> <command> [repeat]"));
      return;
    }

    String addrStr = param.substring(0, space1);
    String cmdStr = param.substring(space1 + 1, space2);
    String repStr = param.substring(space2 + 1);

    uint8_t address = strtoul(addrStr.c_str(), NULL, 0);
    uint8_t command = strtoul(cmdStr.c_str(), NULL, 0);
    uint8_t repeats = strtoul(repStr.c_str(), NULL, 0);

    Serial.print(F("[SENDNEC] Address=0x"));
    Serial.print(address, HEX);
    Serial.print(F(" Command=0x"));
    Serial.print(command, HEX);
    Serial.print(F(" Repeats="));
    Serial.println(repeats);

    IrSender.sendNEC(address, command, repeats);
    Serial.println(F("[SENDNEC] 完成"));
  }
  else if (cmd == "scannec") {
    startScanNEC = true;
    lastScanTime = xTaskGetTickCount(); // 初始化時間
    Serial.println(F("[CMD] 已啟動 NEC 掃描旗標"));
  }
  else {
    Serial.println(F("未知指令！輸入 help"));
  }
}


void printHelp() {
  Serial.println(F("=== 指令 ==="));
  Serial.println(F("read   - 接收2分鐘"));
  Serial.println(F("sendnec <addr> <cmd> [rep] - 發送 NEC 指令, ex: sendnec 0x00 0x46 2"));
  Serial.println(F("scannec - 掃描所有 NEC 指令 (Address=0x00)"));
  Serial.println(F("help   - 說明"));
  Serial.println(F("============"));
}
