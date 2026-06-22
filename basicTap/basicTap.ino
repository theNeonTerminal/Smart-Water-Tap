#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>

// --- Pins ---
const int LED_PIN = 4;
const int SOLENOID_PIN = 19;

// --- Constants & Thresholds ---
const int DISTANCE_THRESHOLD = 110;           // 11 cm in mm (VL53L0X uses mm)
const unsigned long OVERRUN_DELAY = 5000;     // 5 seconds overrun
const unsigned long MAX_ON_DURATION = 60000;  // 1 minute max runtime
const unsigned long WARNING_WINDOW = 10000;   // Flash LED 10s before 1 min limit (at 50s mark)
const unsigned long BREAK_DURATION = 10000;   // 10 seconds lockout break
const unsigned long BLINK_INTERVAL = 250;     // LED flash interval (ms)

// --- System States ---
enum TapState {
  STATE_IDLE,
  STATE_TAP_ON,
  STATE_OVERRUN,
  STATE_LOCKOUT_WAIT_FOR_LEAVE,
  STATE_LOCKOUT_DELAY
};

// --- Shared FreeRTOS Variables ---
volatile bool handDetected = false;
SemaphoreHandle_t xMutex = NULL;

// --- Instantiate Sensor ---
VL53L0X sensor;

// --- Function Prototypes ---
void handleTapLogic();
void readSensorTask(void *pvParameters);
void wirelessTask(void *pvParameters);

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(LED_PIN, OUTPUT);
  pinMode(SOLENOID_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(SOLENOID_PIN, LOW);

  // Initialize VL53L0X
  sensor.setTimeout(500);
  if (!sensor.init()) {
    Serial.println("Failed to detect and initialize VL53L0X!");
    while (1);
  }
  sensor.startContinuous();

  // Create Mutex for shared boolean variable protection
  xMutex = xSemaphoreCreateMutex();

  // Create FreeRTOS Tasks
  xTaskCreatePinnedToCore(readSensorTask, "ReadSensor", 3000, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(wirelessTask, "WirelessComm", 2000, NULL, 1, NULL, 0);
}

void loop() {
  // Main loop handles the tap feedback conditioning state machine
  handleTapLogic();
  delay(10);  // Yield slightly to prevent watchdog starvation
}

// --- Sensor Reading Task (Core 1) ---
void readSensorTask(void *pvParameters) {
  (void)pvParameters;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(1000);  // 1 second intervals

  for (;;) {
    uint16_t distance = sensor.readRangeContinuousMillimeters();
    bool currentDetection = false;

    if (sensor.timeoutOccurred()) {
      Serial.println(" Sensor Timeout!");
    } else {
      // Check if hand is within 11cm threshold
      currentDetection = (distance > 0 && distance <= DISTANCE_THRESHOLD);
    }

    // Safely update global flag
    if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
      handDetected = currentDetection;
      xSemaphoreGive(xMutex);
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// --- Wireless Communication Placeholder Task (Core 0) ---
void wirelessTask(void *pvParameters) {
  (void)pvParameters;
  for (;;) {
    // Background WiFi/BLE operations would live here
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// --- Feedback Conditioning State Machine ---
void handleTapLogic() {
  static TapState currentState = STATE_IDLE;
  static unsigned long tapOnStartTime = 0;
  static unsigned long overrunStartTime = 0;
  static unsigned long lockoutStartTime = 0;
  static unsigned long lastBlinkTime = 0;
  static bool blinkState = false;

  // Local snapshot of the hand detection flag to minimize mutex lock durations
  bool isHandPresent = false;
  if (xSemaphoreTake(xMutex, (TickType_t)10) == pdTRUE) {
    isHandPresent = handDetected;
    xSemaphoreGive(xMutex);
  }

  unsigned long currentMillis = millis();

  switch (currentState) {

    case STATE_IDLE:
      if (isHandPresent) {
        digitalWrite(SOLENOID_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
        tapOnStartTime = currentMillis;
        currentState = STATE_TAP_ON;
        Serial.println("Tap Status: ON");
      }
      break;

    case STATE_TAP_ON:
      // Calculate active running duration
      {
        unsigned long tapOnDuration = currentMillis - tapOnStartTime;

        // Condition A: Hand is removed before 1 minute limit
        if (!isHandPresent) {
          overrunStartTime = currentMillis;
          currentState = STATE_OVERRUN;
          Serial.println("Tap Status: Overrun Active (5s)");
          break;
        }

        // Condition B: Approaching 1 minute limit (Warning Window: last 10 seconds)
        if (tapOnDuration >= (MAX_ON_DURATION - WARNING_WINDOW) && tapOnDuration < MAX_ON_DURATION) {
          if (currentMillis - lastBlinkTime >= BLINK_INTERVAL) {
            lastBlinkTime = currentMillis;
            blinkState = !blinkState;
            digitalWrite(LED_PIN, blinkState);
          }
        }

        // Condition C: Hits exactly 1 minute limit -> Shut down immediately
        if (tapOnDuration >= MAX_ON_DURATION) {
          digitalWrite(SOLENOID_PIN, LOW);
          digitalWrite(LED_PIN, LOW);
          Serial.println("Tap Status: Hard limit reached. Entering Lockout phase.");
          currentState = STATE_LOCKOUT_WAIT_FOR_LEAVE;
        }
      }
      break;

    case STATE_OVERRUN:
      // Hand returned while running out the 5s timer -> Return to ON state
      if (isHandPresent) {
        currentState = STATE_TAP_ON;
        digitalWrite(LED_PIN, HIGH);  // ensure LED solid
        Serial.println("Tap Status: ON (Hand Returned)");
        break;
      }

      // Check if 5 second overrun limit is up
      if (currentMillis - overrunStartTime >= OVERRUN_DELAY) {
        digitalWrite(SOLENOID_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
        currentState = STATE_IDLE;
        Serial.println("Tap Status: OFF");
      }
      break;

    case STATE_LOCKOUT_WAIT_FOR_LEAVE:
      // Force user to pull hand back beyond 11cm threshold before break timer starts
      if (!isHandPresent) {
        lockoutStartTime = currentMillis;
        currentState = STATE_LOCKOUT_DELAY;
        Serial.println("Tap Status: Hand cleared. Starting 10s recovery break.");
      }
      break;

    case STATE_LOCKOUT_DELAY:
      // Ignore reading entirely until 10 seconds elapse
      if (currentMillis - lockoutStartTime >= BREAK_DURATION) {
        currentState = STATE_IDLE;
        Serial.println("Tap Status: Ready again.");
      }
      break;
  }
}
