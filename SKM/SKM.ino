#include <HX711.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// WiFi Credentials
#define WIFI_SSID "M13"
#define WIFI_PASSWORD "11112222"

// Firebase
#define FIREBASE_HOST "smartkitchen-dd50c-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "AIzaSyDl6z4Bd_HWkzkYQ762gilhdXviVXSLfJ8"

// Load cell objects and pins
HX711 scale1, scale2, scale3, scale4;
#define DOUT1 16
#define CLK1 5
#define DOUT2 4
#define CLK2 0
#define DOUT3 2
#define CLK3 14
#define DOUT4 12
#define CLK4 13

// Ultrasonic Sensor
#define TRIG2 15
#define ECHO2 3

// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

float clean(float val) {
  if (val < 0.05 && val > -0.05) return 0.00;
  if (val < 0) return 0.00;
  return val;
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("....");
  Serial.print("Connected: ");
  Serial.println(WiFi.localIP());

  // Firebase setup
  config.database_url = FIREBASE_HOST;
  config.api_key = FIREBASE_AUTH;
  config.token_status_callback = tokenStatusCallback;
  config.signer.test_mode = true;  // Anonymous login

  auth.user.email = "";
  auth.user.password = "";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize load cells
  scale1.begin(DOUT1, CLK1); scale1.set_scale(695086.991); scale1.tare();
  scale2.begin(DOUT2, CLK2); scale2.set_scale(198152.99); scale2.tare();
  scale3.begin(DOUT3, CLK3); scale3.set_scale(165379.544); scale3.tare();
  scale4.begin(DOUT4, CLK4); scale4.set_scale(101459.395); scale4.tare();

  // Ultrasonic sensor
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);

  Serial.println("Setup complete.");
}

long readUltrasonic(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  return pulseIn(echoPin, HIGH);
}

float distanceCM(int trigPin, int echoPin) {
  long duration = readUltrasonic(trigPin, echoPin);
  return duration * 0.034 / 2;
}

float getLiquidLevelPercent(float distance, float tankHeight) {
  float level = tankHeight - distance;
  float percent = (level / tankHeight) * 100.0;
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  return percent;
}

float liter(float level) {
  float volume_cm3 = 3.1416 * 8.3 * 8.3 * level;
  return volume_cm3 / 1000.0;
}

void loop() {
  // Read weights
  float Chill_Powder = clean(scale1.get_units(5));
  float Suger = clean(scale2.get_units(5));
  float Corn_Flour = clean(scale3.get_units(5));
  float Rice = clean(scale4.get_units(5));

  // Read ultrasonic and compute oil level
  float distanceOil = distanceCM(TRIG2, ECHO2);
  float oilHeight = 19.0 - distanceOil;
  if (oilHeight < 0) oilHeight = 0;

  float oil_liter = liter(oilHeight);
  float oilLevelPercent = getLiquidLevelPercent(distanceOil, 19);

  // Print to Serial Monitor
  Serial.print("Chill_Powder: "); Serial.println(Chill_Powder);
  Serial.print("Suger: "); Serial.println(Suger);
  Serial.print("Corn_Flour: "); Serial.println(Corn_Flour);
  Serial.print("Rice: "); Serial.println(Rice);
  Serial.println("\nOil :................");
  Serial.print("Oil Distance (cm): "); Serial.println(distanceOil);
  Serial.print("Oil Height (cm): "); Serial.println(oilHeight);
  Serial.print("Oil Volume (L): "); Serial.println(oil_liter);
  Serial.print("Oil Level (%): "); Serial.println(oilLevelPercent);

  // Upload to Firebase
  if (
    Firebase.RTDB.setFloat(&firebaseData, "/weights/Chill_Powder", Chill_Powder) &&
    Firebase.RTDB.setFloat(&firebaseData, "/weights/Suger", Suger) &&
    Firebase.RTDB.setFloat(&firebaseData, "/weights/Corn_Flour", Corn_Flour) &&
    Firebase.RTDB.setFloat(&firebaseData, "/weights/Rice", Rice) &&
    Firebase.RTDB.setFloat(&firebaseData, "/weights/oil_liter", oil_liter)
    // Firebase.RTDB.setFloat(&firebaseData, "/weights/Oil", oilLevelPercent)
  ) {
    Serial.println("All weights uploaded successfully\n");
  } else {
    Serial.print("Upload failed: ");
    Serial.println(firebaseData.errorReason());
  }

  delay(5000);  // Wait 5 seconds before next read
}
