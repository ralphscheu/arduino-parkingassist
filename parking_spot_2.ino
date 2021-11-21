// LED
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
// Signal Pin - Im Shield fest verdrahtet
#define PIN 4
// Grundeinstellung (Anzahl der LEDs, usw.)
// Shield mit 40 LEDs, Streifen mit 8 LEDs
Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, PIN, NEO_GRB + NEO_KHZ800);
#define GREEN strip.Color(0, 255, 0)
#define BRIGHT_ORANGE strip.Color(255, 190, 0)
#define ORANGE strip.Color(255, 127, 0)
#define DARK_ORANGE strip.Color(255, 63, 0)
#define RED strip.Color(255, 0, 0)
#define BLUE strip.Color(0, 0, 255)
// Schmitt-Trigger LED
bool green_high = false;
bool bright_orange_high = false;
bool dark_orange_high = false;
bool red_high = false;


// Ultraschall
#define Echo_EingangsPin 7 // Echo Eingangs-Pin
#define Trigger_AusgangsPin 8 // Trigger Ausgangs-Pin
// Benoetigte Variablen werden definiert
int maximumRange = 300;
int minimumRange = 2;
long Abstand;
long Dauer;


// Pretty Printing
byte print_dist = 0;
String car_top = ".-'--`-._";
String car_bottom = "'-O---O--'";
String wall = "||";


// Main Function
#include <SAMDTimerInterrupt.h>
#include <SAMD_ISR_Timer.h>
SAMDTimer ITimer0(TIMER_TC3);
long timer_time = 1;
int time_array[10];
int driving_sum = 0;
unsigned long start_t, stop_t = 0;
enum States {FREE, OCCUPIED, PARKING_IN, PARKING_OUT, RESERVED};
States state = FREE;
long distance = 0;
long dist_array[5];
byte driving = 0;
bool change = true;
States last_state = state;


// MQTT
#include <WiFiNINA.h>                     // Wifi Client Library für Nano 33 IoT Board
#include <PubSubClient.h>                 // MQTT Bibliothek, ggf. über Sketch / Include Librar / Manage Libraries nachinstallieren
char ssid[] = "mico-b-wifi";              //  todo
char pass[] = "Mico-B-Wifi";          // todo
char mqttUserName[] = "MicoBUser";        // Irgendein Nutzer name
char mqttPass[] = "ACEZBT00WA7NHCC5";       // todo, MQTT Api Key, siehe User / My Profile
char readAPIKey[] = "6S86CZG5SZXQMYDK";     // todo, Read API Key, siehe Channel / API Keys
long channelID = 1570228;                  // todo, Numerische Channel Id, Siehe Cannel
int fieldNumber = 1;                      // todo, Field Number
unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 0.5 * 1000L;  // Post data every 10 seconds
WiFiClient client;                        // Wifi Client
PubSubClient mqttClient(client);          // MQTT Client, an Wifi gebunden



/********** [ LED FUNCTIONS ] ********************************************************************

    3 Functions defined:
    1. void led_connect(color)
      display a (blue) light during WIFI connection for user feedback
    2. void led_const(color)
      constant light to display the states FREE (green), OCCUPIED (RED), RESERVED (orange)
    3. void led_dist(distance)
      display the distance of the car with green to red lights
*/


void led_connect(uint32_t color) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(50);
  }
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
    strip.show();
  }
}


void led_const(uint32_t color) {
  // constant green light
  for (byte i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
    strip.show();
  }
}

void led_dist(long dist) {
  strip.setPixelColor(0, GREEN);
  strip.setPixelColor(7, GREEN);
  if (not bright_orange_high and dist <= 90) {        // Schmitt-Trigger to avoid light flickering
    strip.setPixelColor(1, BRIGHT_ORANGE);            //    High____________________
    strip.setPixelColor(6, BRIGHT_ORANGE);            //            \               \
    bright_orange_high = true;                        //             \      10cm     \
  } else if (bright_orange_high and dist <= 100) {    //              \               \
    strip.setPixelColor(1, BRIGHT_ORANGE);            //               \_______________\______Low
    strip.setPixelColor(6, BRIGHT_ORANGE);
  } else {
    strip.setPixelColor(1, 0);
    strip.setPixelColor(6, 0);
    bright_orange_high = false;
  }
  if (not dark_orange_high and dist <= 70) {
    strip.setPixelColor(2, DARK_ORANGE);
    strip.setPixelColor(5, DARK_ORANGE);
    dark_orange_high = true;
  } else if (dark_orange_high and dist <= 80) {
    strip.setPixelColor(2, DARK_ORANGE);
    strip.setPixelColor(5, DARK_ORANGE);
  } else
  {
    strip.setPixelColor(2, 0);
    strip.setPixelColor(5, 0);
    dark_orange_high = true;
  }
  if (not red_high and dist <= 40) {
    strip.setPixelColor(3, RED);
    strip.setPixelColor(4, RED);
    red_high = true;
  } else if (red_high and dist <= 50) {
    strip.setPixelColor(3, RED);
    strip.setPixelColor(4, RED);
  } else {
    strip.setPixelColor(3, 0);
    strip.setPixelColor(4, 0);
    red_high = false;
  }
  if (dist <= 20) {
    for (byte i = 0; i < strip.numPixels(); i++)
      strip.setPixelColor(i, RED);
  }
  strip.show();
}


/********** [ ULTRA SOUND FUNCTIONS ] ************************************************************

   2 functions defined:
   1. int ultra_sound()
    returns the distance to the car in cm
   2. byte motion(distance)
    calculates if the car moves, by comparing the 5 last distance measurements
    - 0 no movement
    - 1 backwards
    - 255 forwards
*/


int ultra_sound() {
  // return distance in cm
  // Abstandsmessung wird mittels des 10us langen Triggersignals gestartet
  digitalWrite(Trigger_AusgangsPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trigger_AusgangsPin, LOW);
  // Nun wird am Echo-Eingang gewartet, bis das Signal aktiviert wurde
  // und danach die Zeit gemessen, wie lang es aktiviert bleibt
  Dauer = pulseIn(Echo_EingangsPin, HIGH);
  // Nun wird der Abstand mittels der aufgenommenen Zeit berechnet
  Abstand = Dauer / 58.2;
  // Überprüfung ob gemessener Wert innerhalb der zulässingen Entfernung liegt
  if (Abstand >= maximumRange) {
    // Falls nicht wird eine Fehlermeldung ausgegeben.
    Serial.println("Abstand ausserhalb des Messbereichs");
    Serial.println("-----------------------------------");
  }
  else if ( Abstand <= minimumRange) {
    Serial.println("!!! CRASH !!!");
    Serial.println("-----------------------------------");
  }
  else {
    // Der berechnete Abstand wird in der seriellen Ausgabe ausgegeben
    Serial.print("Der Abstand betraegt:");
    Serial.print(Abstand);
    Serial.println("cm");
    Serial.println("-----------------------------------");
  }
  return Abstand;

}


byte motion(long distance) {
  // shift the distance_array
  if (distance < 100) {
    for (byte i = 4; i > 0; i--)
      dist_array[i] = dist_array[i - 1];
    dist_array[0] = distance;
    if (dist_array[0] > dist_array[1] and dist_array[1] > dist_array[2] and dist_array[1] > dist_array[3]) // every consecutive distance is greater than before
      driving = 1; // car is moving out of the spot
    else if (dist_array[0] < dist_array[1] and dist_array[1] < dist_array[2] and dist_array[1] < dist_array[3]) // every consecutive distance is shorter than before
      driving = -1; // car is moving into the spot
    else
      driving = 0; // car is not moving
  }
  else // the distance measurement is not stable for large distances and will therefore be ignored => no movement
    driving = 0;
  return driving;
}


/********** [TIMER FUNCTIONS ] **********************************************************************

    1 function defined:
    check_time()
      changes the state to FREE or OCCUPIED depending on the motion of the car for the last 10 seconds
*/

void check_time() {
  // shift the time_array
  driving_sum = 0;
  for (byte i = 9; i > 0; i--) {
    time_array[i] = time_array[i - 1];
    driving_sum += time_array[i];
  }
  time_array[0] = driving;
  if (((driving_sum + driving) == 0) and state != RESERVED) { // if there is no movement during the last 10 seconds (see Timer settings) and the car is not reserved:
    if (distance < 100)
      state = OCCUPIED;
    else
      state = FREE;
  }
}


/********** [ MQTT FUNCTIONS ] *******************************************************************

    3 functions defined
    1. void reserve_callback(topic, payload, legngth)
      listens to the state topic and changes the state to RESERVED or FREE; The topic is only published
      when a reservation is requested or the reservation time is up. Reservations are only possible, when
      the current state is FREE.
    2. void reconnect()
      reestablishes the connection if it gets lost
    3. void mqttPublishDistAndState(distance, state)
      publishes the current distance and state
*/


void reserve_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  if (payload[0] == 'R') // only need to check the first byte
    state = RESERVED;
  else if (payload[0] == 'F') // only need to check the first byte
    state = FREE;
  Serial.println();
}


void reconnect() {
  // Loop until reconnected.
  while (!mqttClient.connected()) {
    led_connect(BLUE); // LED user feedback
    // Zufällige Client Id aus User Name und Zufallszahl.
    String clientID = String(mqttUserName) + "_" + String( random(), HEX );
    Serial.print("Attempting MQTT connection...");
    //Serial.print("ClientId="); Serial.println(clientID);

    // Connect to the MQTT broker.
    if (mqttClient.connect(clientID.c_str(), mqttUserName, mqttPass)) {
      Serial.print("connected - subscribe to ");
      String topic = "parkingassist/spot01/#";
      Serial.println(topic);
      mqttClient.subscribe(topic.c_str());
    } else {
      Serial.print("failed, rc=");
      // Print reason the connection failed.
      // See https://pubsubclient.knolleary.net/api.html#state for the failure code explanation.
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      for (byte i = 0; i < 5; i++) {
        led_connect(BLUE); // LED user feedback
        delay(1000); // Warte vor einem neuen Versuch oder einem Zugriff
      }
    }
  }
}


void mqttPublishDistAndState(int dist, States s) {
  //
  String dist_data = String(dist, DEC);
  String state_data = print_state(s, false);

  // Topic anhand der Einstellungen und der Feld Nummer erstellen
  String dist_topic = "parkingassist/spot01/car_distance";
  String state_topic = "parkingassist/spot01/status";

  Serial.print("Publish "); Serial.print(dist_data); Serial.print(" to "); Serial.println(dist_topic);
  Serial.print("Publish "); Serial.print(state_data); Serial.print(" to "); Serial.println(state_topic);

  // Publish. Take care pass C char array instead of the String object
  mqttClient.publish( dist_topic.c_str(), dist_data.c_str() );
  if (state != last_state)
    mqttClient.publish( state_topic.c_str(), state_data.c_str() ); // only publish the state when it changes (communication delay)
  last_state = state;
}


/********** [ PRETTY PRINTING ] ******************************************************************
 *  
 *  3 functions defined:
 *  1. String print_state(state, print_out)
 *    returns a string of the current state and prints it to the Serial Monitor if print_out == true
 *  2. void print_distance(distance)
 *    prints the current distance to the Serial Monitor
 *  3. void print_car(distance)
 *    visualization of a small car parking depending on the distance
*/


String print_state(States s, bool print_out) {
  String state_string;
  switch (s) {
    case FREE:
      {
        state_string = "FREE";
      }
      break;
    case OCCUPIED:
      {
        state_string = "OCCUPIED";
      }
      break;
    case PARKING_IN:
      {
        state_string = "PARKING_IN";
      }
      break;
    case PARKING_OUT:
      {
        state_string = "PARKING_OUT";
      }
      break;
    case RESERVED:
      {
        state_string = "RESERVED";
      }
      break;
    default:
      {
        state_string = "ERROR";
      }
  }
  if (print_out) {
    Serial.print("Current State: ");
    Serial.println(state_string);
  }
  return state_string;
}


void print_distance(int distance) {
  if (distance >= maximumRange) {
    // Falls nicht wird eine Fehlermeldung ausgegeben.
    Serial.println("Abstand ausserhalb des Messbereichs");
  }
  else if (distance <= minimumRange) {
    Serial.println("ACHTUNG UNFALL");
  }
  else {
    // Der berechnete Abstand wird in der seriellen Ausgabe ausgegeben
    Serial.print("Der Abstand betraegt:");
    Serial.print(distance);
    Serial.println("cm");
  }
}


void print_car(int distance) {
  if (distance > 100)                                         //   top     ||        .-'--`-._
    print_dist = 100;  // print only up to 100cm (symbols)    //   bottom  ||________'-O---O--'_______
  else
    print_dist = distance;
    
  // top line
  Serial.print(wall);
  for (byte i = 0; i < print_dist; i++)
    Serial.print(" ");
  if (distance < 100) {
    Serial.print(car_top);
    for (byte i = 0; i < 100 - print_dist; i ++)
      Serial.print(" ");
  }
  Serial.println("");
  
  // bottom line
  Serial.print(wall);
  for (byte i = 0; i < print_dist; i++)
    Serial.print("_");
  if (distance < 100) {
    Serial.print(car_bottom);
    for (byte i = 0; i <= 100 - print_dist; i ++)
      Serial.print("_");
  }
  Serial.println("");
}


/********** [ SETUP ] ****************************************************************************/


void setup() {
  
  // Serial port
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // Ultrasound
  pinMode(Trigger_AusgangsPin, OUTPUT);
  pinMode(Echo_EingangsPin, INPUT);
  
  // LED
  strip.begin();
  strip.show(); // Alle LEDs initialisieren

  // initialize the state
  distance = ultra_sound();
  if (distance < 100)
    state = OCCUPIED;

  // Timer
  if (ITimer0.attachInterruptInterval(timer_time * 1000000, check_time))
    Serial.println("Starting  ITimer0 OK, millis() = " + String(millis()));
  else
    Serial.println("Can't set ITimer0. Select another freq. or timer");

  // MQTT
  int status_wifi = WL_IDLE_STATUS;  // Initialisiere für folgende Schleife
  // Attempt to connect to Wi-Fi network.
  while (status_wifi != WL_CONNECTED)
  {
    Serial.println("Try WiFi connection ...");
    status_wifi = WiFi.begin(ssid, pass); // Connect to WPA/WPA2 Wi-Fi network.
    for (byte i = 0; i < 5; i++) {
      led_connect(BLUE);
      delay(1000); // Warte vor einem neuen Versuch oder einem Zugriff
    }
  }
  Serial.println("Connected to wifi");
  mqttClient.setServer("192.168.0.119", 11883);   // Set the MQTT broker details.
  mqttClient.setCallback(reserve_callback); // Callback Funktion registrieren ... ganz unten ...
}


/********** [ MAIN ] *****************************************************************************/


void loop() {
  // put your main code here, to run repeatedly:
  distance = ultra_sound();
  driving = motion(distance);
  print_state(state, true);
  print_distance(distance);
  print_car(distance);

  delay(200);

  switch (state) {   // state machine
    case FREE:
      led_const(GREEN);
      if (driving == 255) { // parking in
        for (byte i = 0; i < 10; i++) time_array[i] = 255; // Timer has an empty array and would immediatly (for one cycle) set to occupied because ofdistance
        state = PARKING_IN;  // change state
      }
      break;
    case PARKING_IN:
      led_dist(distance);  // state change through Timer
      break;
    case PARKING_OUT:      // state change through Timer
      led_dist(distance);
      break;
    case OCCUPIED:
      led_const(RED);
      if (driving == 1) {
        for (byte i = 0; i < 10; i++) time_array[i] = 1;  // to avoid the timer from instantly changing the state
        state = PARKING_OUT;
      }
      if (driving == 255) {
        for (byte i = 0; i < 10; i++) time_array[i] = 255;  // to avoid the timer from instantly changing the state
        state = PARKING_IN;
      }
      break;
    case RESERVED:
      led_const(ORANGE);
      if (driving == 255) { // parking in
        for (byte i = 0; i < 10; i++) time_array[i] = 255;  // to avoid the timer from instantly changing the state
        state = PARKING_IN;  // change state
      }
      break;
  }
  if (!mqttClient.connected())
  {
    reconnect();
  }
  mqttClient.loop();
  mqttPublishDistAndState((int) distance, state);
}
