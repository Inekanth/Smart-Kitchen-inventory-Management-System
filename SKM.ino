#include <HX711.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h> // For token generation
#include <addons/RTDBHelper.h>  // For RTDB functions


// WiFi Credentials
#define WIFI_SSID "Dialog 4G 2EF"
#define WIFI_PASSWORD "945H0FJ7N66"

// Firebase Credentials
#define FIREBASE_HOST "uwu-ict-13-default-rtdb.asia-southeast1.firebasedatabase.app" // NO "https://"
#define FIREBASE_AUTH "t4Xok6mQgnN8jdsjugg1cpyB64BkuZ8Zx3FL6uaz"

// HX711 Load Cell Objects
HX711 scale1, scale2, scale3, scale4;

// HX711 Pins (GPIO)
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
#define ECHO2 3  // WARNING: GPIO1 is TX. Be careful.



// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("....");
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  // Firebase configuration
  config.api_key = "AIzaSyATVTm2wQrQNZ5vOgEJuBIvgwejz3hySBo";
  config.database_url = "https://uwu-ict-13-default-rtdb.asia-southeast1.firebasedatabase.app";
  config.token_status_callback = tokenStatusCallback; // Add this line

  // Anonymous sign-in (no email/password)
  auth.user.email = "lingarashainekanth99@gmail.com";
  auth.user.password = "Core@123#321";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println(firebaseData.errorReason());
  Serial.println("Success");

  // Initialize HX711
  // Replace with your actual calibration values after tuning
  #define CALIBRATION_FLOUR     -7050.01
  #define CALIBRATION_VEGETABLE -7048.8
  #define CALIBRATION_CHICKEN   -7050.0
  #define CALIBRATION_RICE      -7050.0

  scale1.begin(DOUT1, CLK1); scale1.set_scale(CALIBRATION_FLOUR); scale1.tare();
  scale2.begin(DOUT2, CLK2); scale2.set_scale(CALIBRATION_VEGETABLE); scale2.tare();
  scale3.begin(DOUT3, CLK3); scale3.set_scale(CALIBRATION_CHICKEN); scale3.tare();
  scale4.begin(DOUT4, CLK4); scale4.set_scale(CALIBRATION_RICE); scale4.tare();


  // Initialize Ultrasonic sensor
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);


  Serial.println("Setup complete.");
}


long readUltrasonic(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  return pulseIn(echoPin, HIGH);
}

float distanceCM(int trigPin, int echoPin) {
  long duration = readUltrasonic(trigPin, echoPin);
  float distance = duration * 0.034 / 2;
  return distance;
}

float getLiquidLevelPercent(float distance, float tankHeight) {
  float level = tankHeight - distance;
  float percent = (level / tankHeight) * 100.0;
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  return percent;
}


void loop() {
  
  float Flour = scale1.get_units(5);
  float Vagetable = scale2.get_units(5);
  float Chicken = scale3.get_units(5);
  float Rice = scale4.get_units(5);


  float distanceOil = distanceCM(TRIG2, ECHO2);
  float oilLevelPercent = getLiquidLevelPercent(distanceOil, 15.0);


  Serial.print("Flour: "); Serial.println(Flour);
  Serial.print("Vagetable: "); Serial.println(Vagetable);
  Serial.print("Chicken: "); Serial.println(Chicken);
  Serial.print("Rice: "); Serial.println(Rice);
  
  Serial.print("Oil Distance: "); Serial.println(distanceOil);
  Serial.print("Oil Level (%): "); Serial.println(oilLevelPercent);

  if (
    Firebase.RTDB.setFloat(&firebaseData, "/weights/Flour", Flour)&&
    Firebase.RTDB.setFloat(&firebaseData, "/weights/Vagetable", Vagetable)&&
    Firebase.RTDB.setFloat(&firebaseData, "/weights/Chicken", Chicken)&&
    Firebase.RTDB.setFloat(&firebaseData, "/weights/Rice", Rice)&&
    Firebase.RTDB.setFloat(&firebaseData, "/weights/Oil", oilLevelPercent)
  ){
    Serial.println("All weights uploaded successfully");
  } else {
  Serial.print("Upload failed: ");
  Serial.println(firebaseData.errorReason());
  }


  delay(5000);
}