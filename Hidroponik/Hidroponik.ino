#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <AntaresESPHTTP.h>

// Antares and Wi-Fi configuration
#define ACCESSKEY "acceskey antares"  // Antares account access key
#define WIFISSID "wifiid"         // Wi-Fi SSID to connect to
#define PASSWORD "password wifi"    // Wi-Fi password
#define projectName "nameproject antares"   // Name of the application created in Antares
#define deviceName "device project"     // Name of the device created in Antares

AntaresESPHTTP antares(ACCESSKEY);

// LCD configuration
int lcdColumns = 20;
int lcdRows = 4;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

// Sensor configuration
const int trigPin = D6; // Pin TRIG pada ESP8266
const int echoPin = D7; // Pin ECHO pada ESP8266
int pHSense = A0;      // Pin untuk sensor pH
int tdsSense = A0;     // Pin untuk sensor TDS (gunakan A0 karena sensor analog)
int samples = 10; // Jumlah sampel untuk pembacaan TDS, ditingkatkan untuk hasil yang lebih stabil

#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

long duration;
int distanceCm;
int distanceInch;

const float tdsConversionFactor = 0.5; // Menggunakan faktor konversi yang sama

void setup() {
    // Initialize Serial Monitor
    Serial.begin(115200);

    // Initialize I2C communication and LCD
    Wire.begin(D14, D15); // SDA on D14, SCL on D15
    lcd.init();
    lcd.backlight(); // turn on LCD backlight
    
    // Initialize sensor pins
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    // Connect to Wi-Fi
    WiFi.begin(WIFISSID, PASSWORD);
    Serial.print("Connecting to Wi-Fi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to Wi-Fi");

    // Initialize connection to Antares using HTTP
    antares.setDebug(true); // Enable debugging
    antares.wifiConnection(WIFISSID, PASSWORD);
}

float calculatePH(float voltage) {
    const float pHNeutral = 7.0;
    const float voltageNeutral = 1.65;
    const float voltagePerPHUnit = 0.059;
    return ((voltage - voltageNeutral) / voltagePerPHUnit + pHNeutral) / 2;
}

int readTDS() {
    long tdsSum = 0;
    for (int i = 0; i < samples; i++) {
        tdsSum += analogRead(tdsSense);  // Membaca nilai analog dari sensor TDS
        delay(10);
    }
    int rawTDSValue = tdsSum / samples;  // Menghitung rata-rata nilai TDS
    return rawTDSValue * tdsConversionFactor;  // Menghitung nilai TDS berdasarkan faktor konversi
}

void loop() {
    long measurings = 0;
    for (int i = 0; i < samples; i++) {
        measurings += analogRead(pHSense);
        delay(10);
    }
    float voltage = (3.3 / 1023.0) * measurings / samples; // A0 pada ESP8266 0-3.3V

    // Ultrasound distance measurement
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    distanceCm = duration * SOUND_SPEED / 2;
    distanceInch = distanceCm * CM_TO_INCH;

    int tdsValue = readTDS();  // Membaca nilai TDS dari sensor
    float pHValue = calculatePH(voltage);  // Menghitung nilai pH dari pembacaan sensor pH

    // Display on LCD
    lcd.setCursor(0, 0);
    lcd.print("Jarak: ");
    lcd.print(distanceCm);
    lcd.print(" cm");
    lcd.setCursor(0, 1);
    lcd.print("pH: ");
    lcd.print(pHValue, 2);
    lcd.setCursor(0, 3);
    lcd.print("TDS: ");
    lcd.print(tdsValue);

    // Display on Serial Monitor
    Serial.print("Distance: ");
    Serial.print(distanceCm);
    Serial.println(" cm");
    Serial.print("pH: ");
    Serial.print(pHValue, 2);
    Serial.println();
    Serial.print("TDS: ");
    Serial.println(tdsValue);

    // Sending data to Antares using HTTP
    antares.add("distance", distanceCm);
    antares.add("ph", pHValue);
    antares.add("tds", tdsValue);
    antares.sendNonSecure(projectName, deviceName);  // Changed to sendNonSecure

    delay(6000);
    lcd.clear();
}
