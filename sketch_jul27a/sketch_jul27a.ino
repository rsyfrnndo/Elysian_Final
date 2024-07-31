#define TdsSensorPin1 A0 // ESP32 pin number for TDS sensor 1 (A0)
#define TdsSensorPin2 A3 // ESP32 pin number for TDS sensor 2 (A3)
#define VREF 3.3          // Analog reference voltage (Volt) of the ADC
#define SCOUNT  30        // Sum of sample points
#define MEASURE_DURATION 5000 // Measurement duration in milliseconds

int analogBuffer1[SCOUNT];     // Store the analog value in the array, read from ADC
int analogBuffer2[SCOUNT];
int analogBufferTemp1[SCOUNT];
int analogBufferTemp2[SCOUNT];
int analogBufferIndex1 = 0;
int analogBufferIndex2 = 0;
int copyIndex = 0;
int dataIndx = 0;

float averageVoltage1 = 0;
float averageVoltage2 = 0;
float tdsValue1 = 0;
float tdsValue2 = 0;
float temperature = 25;       // Current temperature for compensation

bool isMeasuring1 = false;
bool isMeasuring2 = false;
unsigned long startTime1 = 0;
unsigned long startTime2 = 0;

int tdsSamples1[SCOUNT];
int tdsSamples2[SCOUNT];
int sampleCount1 = 0;
int sampleCount2 = 0;

// Median filtering algorithm
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0) {
    bTemp = bTab[(iFilterLen - 1) / 2];
  } else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}

void setup() {
  Serial.begin(115200);
  pinMode(TdsSensorPin1, INPUT);
  pinMode(TdsSensorPin2, INPUT);
}

void loop() {
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {     // Every 40 milliseconds, read the analog value from the ADC
    analogSampleTimepoint = millis();
    analogBuffer1[analogBufferIndex1] = analogRead(TdsSensorPin1);    // Read the analog value and store it into the buffer
    analogBuffer2[analogBufferIndex2] = analogRead(TdsSensorPin2);    // Read the analog value and store it into the buffer
    analogBufferIndex1++;
    analogBufferIndex2++;
    if (analogBufferIndex1 == SCOUNT) {
      analogBufferIndex1 = 0;
    }
    if (analogBufferIndex2 == SCOUNT) {
      analogBufferIndex2 = 0;
    }
  }

  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U) {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++) {
      analogBufferTemp1[copyIndex] = analogBuffer1[copyIndex];
      analogBufferTemp2[copyIndex] = analogBuffer2[copyIndex];
    }

    // Sensor 1
    averageVoltage1 = getMedianNum(analogBufferTemp1, SCOUNT) * (float)VREF / 4096.0;
    float compensationCoefficient1 = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage1 = averageVoltage1 / compensationCoefficient1;
    tdsValue1 = (133.42 * compensationVoltage1 * compensationVoltage1 * compensationVoltage1 
                - 255.86 * compensationVoltage1 * compensationVoltage1 
                + 857.39 * compensationVoltage1) * 0.5;

    // Sensor 2
    averageVoltage2 = getMedianNum(analogBufferTemp2, SCOUNT) * (float)VREF / 4096.0;
    float compensationCoefficient2 = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage2 = averageVoltage2 / compensationCoefficient2;
    tdsValue2 = (133.42 * compensationVoltage2 * compensationVoltage2 * compensationVoltage2 
                - 255.86 * compensationVoltage2 * compensationVoltage2 
                + 857.39 * compensationVoltage2) * 0.5;

    // Check Sensor 1
    if (tdsValue1 > 0) {
      if (!isMeasuring1) {
        isMeasuring1 = true;
        startTime1 = millis();
        sampleCount1 = 0;
      }

      if (isMeasuring1) {
        tdsSamples1[sampleCount1] = tdsValue1;
        sampleCount1++;

        if (millis() - startTime1 >= MEASURE_DURATION) {
          int medianTds1 = getMedianNum(tdsSamples1, sampleCount1);
          Serial.print("Sensor 1 TDS Value (Median): ");
          Serial.print(medianTds1, 0);
          Serial.print(" ppm, Voltage: ");
          Serial.print(averageVoltage1, 2);
          Serial.println(" V");

          sendDataToLocalhost("before", dataIndx, tdsValue1);

          isMeasuring1 = false;
        }
      }
    }

    // Check Sensor 2
    if (tdsValue2 > 0) {
      if (!isMeasuring2) {
        isMeasuring2 = true;
        startTime2 = millis();
        sampleCount2 = 0;
      }

      if (isMeasuring2) {
        tdsSamples2[sampleCount2] = tdsValue2;
        sampleCount2++;

        if (millis() - startTime2 >= MEASURE_DURATION) {
          int medianTds2 = getMedianNum(tdsSamples2, sampleCount2);
          Serial.print("Sensor 2 TDS Value (Median): ");
          Serial.print(medianTds2, 0);
          Serial.print(" ppm, Voltage: ");
          Serial.print(averageVoltage2, 2);
          Serial.println(" V");

          sendDataToLocalhost("after", dataIndx, tdsValue2);

          isMeasuring2 = false;
        }
      }
    }
  }
}


void sendDataToLocalhost(char endpoint, int indx, int ppm) {
  if (WiFi.status() == WL_CONNECTED) { // Check Wi-Fi connection
    HTTPClient http;
    http.begin("http://localhost:8000/" + String(endpoint)); // Replace with your server's endpoint

    // Prepare JSON payload
    String jsonPayload = "{";
    jsonPayload += "\"timestamp\": " + String(millis()) + ",";
    jsonPayload += "\"id\": " + String(indx) + ",";
    jsonPayload += "\"ppm\": " + String(ppmBefore) + ",";
    jsonPayload += "}";

    http.addHeader("Content-Type", "application/json"); // Specify content-type header
    int httpResponseCode = http.POST(jsonPayload);      // Send POST request

    if (httpResponseCode > 0) {
      String response = http.getString();               // Get the response to the request
      Serial.println(httpResponseCode);                 // Print return code
      Serial.println(response);                         // Print request answer
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end(); // Free resources
  } else {
    Serial.println("Error in WiFi connection");
  }
}
