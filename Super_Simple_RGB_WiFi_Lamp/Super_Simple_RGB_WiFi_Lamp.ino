// Included Libraries
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include "user_interface.h"
#include <FastLED.h>
#include "FS.h"
#include <ESP8266WiFi.h>
#include "IPAddress.h"
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESPAsyncTCP.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <ESPAsyncUDP.h>
#include "arduinoFFT.h"
#include "lwip/inet.h"
#include "lwip/dns.h"
#include <map>
#include "FastLED_RGBW.h"


// ############################################################# Sketch Variables #############################################################
// All variables at the top of this sketch need to be defined correctly for your light. Read the comments around each one for more details on 
// what each of them are.

#define DEFAULT_NAME "Lillys Lamp"

// Set Your Data pin - This is the pin on your ESP8266 that is connected to the LED's. Remember to add the letter "D" infront of the number 
// to map the pin correctly for your platform.
#define DATA_PIN D5

// Pin used for the manual switch to turn LEDs on and off
#define SWITCH_PIN D0

// Set the number of LED's - Simply count how many there are on your string and enter the number here.
#define NUM_LEDS 109

// Set your UTC offset - This is the time zone you are in. for example +10 for Sydney or -4 for NYC.
#define UTC_OFFSET 0

// Set the chipset and color order for the LEDs you are using. For more info on supported hardware see: https://github.com/FastLED/FastLED/wiki/Overview#supported-chipsets.
#define CHIPSET WS2812B
#define COLOR_ORDER RGB

// Limit the maximum frame rate to prevent flickering. Values around 400 or
// above cause flickering LEDs because of the WS2821 update frequency.
#define FRAME_RATE 60

// Set up LED's for each side - These arrays hold which leds are on what sides. For the basic rectangular shape in the example this relates to 4
// sides and 4 arrays. You must subract 1 off the count of the LED when entering it as the array is 0 based. For example the first LED on the 
// string is entered as 0.
int topLeds[]     = {52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95};
int bottomLeds[] =  {40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 108, 107};
int leftLeds[]    = {96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106};
int rightLeds[]   = {41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

// Eneter your wifi credentials here - If you would like to enter your wifi credentials now you can with these variables. This is a nice easy 
// method to get your ESP8266 connected to your network quickly. If you don't you can always set it up later in the wifi portal.
String SSID = "";
String Password = "";
// ########################################################## End of Sketch Variables ##########################################################

class ModeBase
{
public:
    /// Override this to initialize state specific variables
    virtual void initialize();

    // Is called once per frame to update the LEDs
    virtual void render();

    // Update config member variables based on the handed over settings
    virtual void applyConfig(JsonVariant& settings);
};

std::map<String, ModeBase*> modes;

// In some cases the automatic creation of the prototypes does not work. Do it manually...
// Config.ino
bool checkFlashConfig();
void getConfig();
bool sendConfigViaWS();
void saveConfigItem(JsonDocument& jsonSetting);
void parseConfig(JsonDocument& jsonMessage);
void addLampInfo(JsonDocument& jsonMessage);
// LEDs.ino
void ledStringInit();
void ledModeInit();
void handleMode();
void adjustBrightnessAndSwitchMode();
// NTP.ino
void handleNTP();
bool getNTPServerIP(const char *_ntpServerName, IPAddress &_ntpServerIp);
bool sendNTPRequest();
void parseNTPResponse(uint8_t *_ntpData);
String get12hrAsString();
// Web_Server.ino
void webServerInit();
void serve404();
void servePage();
void scanForNetworks();
void updateWifiConfigTable(int _numNetworks);
void otaInit();
// Websockets.ino
void websocketsInit();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
bool websocketSend(JsonDocument& jsonMessage);
bool updateClients();
// Wifi.ino
void wifiInit();
void handleWifiConnection();
void onWifiConnected(const WiFiEventStationModeGotIP &event);
void onWifiDisconnected(const WiFiEventStationModeDisconnected &event);
void mdnsInit();

// File System Variables 
bool spiffsCorrectSize      = false;

// Wifi Variables and Objects 
String programmedSSID       = SSID;
String programmedPassword   = Password;
bool wifiStarting           = false;
bool softApStarted          = false;
IPAddress accessPointIP     = IPAddress(192, 168, 1, 1);
WiFiEventHandler stationConnectedHandler;
WiFiEventHandler stationDisconnectedHandler;

// DNS and mDNS Objects
DNSServer captivePortalDNS;
MDNSResponder::hMDNSService mdnsService;

// Webserver and OTA Objects
ESP8266WebServer restServer(80);
ESP8266HTTPUpdateServer OTAServer;

// Web Sockets Variabels and Objects
WebSocketsServer webSocket(81);
bool processingMessage = false;
bool clientNeedsUpdate = false;
bool webSocketConnecting = false;

// NTP Variables and Objects
AsyncUDP udpClient;
bool ntpTimeSet                       = false;
String ntpHostName                    = "pool.ntp.org";
IPAddress ntpIpAddress                = IPAddress(0, 0, 0, 0);
unsigned long utcOffset               = UTC_OFFSET * 3600; // in seconds
unsigned long collectionPeriod        = 3600;
unsigned long currentEpochTime        = 0;
unsigned long lastNTPCollectionTime   = 0;

// LED string object and Variables
CRGBW ledString[NUM_LEDS];
CRGB *ledsRGB = (CRGB *) &ledString[0];
bool autoOnWithModeChange = true;
int topNumLeds      = sizeof(topLeds) / sizeof(*topLeds);
int bottomNumLeds   = sizeof(bottomLeds) / sizeof(*bottomLeds);
int leftNumLeds     = sizeof(leftLeds) / sizeof(*leftLeds);
int rightNumLeds    = sizeof(rightLeds) / sizeof(*rightLeds);

// Base Variables of the Light
String  Name                  = DEFAULT_NAME;                         // The default Name of the Device
String  Mode                  = "";                                   // The default Mode of the Device
bool    State                 = true;                                 // The Default Mode of the Light
int     FadeTime              = 200;                                  // Fading time between states in ms
String  currentMode           = Mode;                                 // Placeholder variable for changing mode
String  previousMode          = "";                                   // Placeholder variable for changing mode
bool    previousState         = false;                                // Placeholder variable for changing state
float   modeChangeFadeAmount  = 0;                                    // Place holder for global brightness during mode change
String  SketchName            = __FILE__;                             // Name of the sketch file (used for info page)

//switch debounce variables
int buttonState;                                                       // the current state from the input pin
int lastButtonState = HIGH;                                            // the previous reading from the input pin
unsigned long lastDebounceTime = 0;                                    // the last time the output pin was toggled
unsigned long debounceDelay = 50;                                      // the debounce time; increase if the output flickers
unsigned long lastTransistionTime = 0;                                 // last time the state changed
unsigned long stateTransistionDelay = 1500;                            // if the button is held down then leave a second between turning on and off

void intCallback(){
  // read the state of the switch into a local variable:
  int reading = digitalRead(SWITCH_PIN);

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
lastButtonState = reading;  
}

void checkSwitchState(){
  // read the state of the switch into a local variable:
  int reading = digitalRead(SWITCH_PIN);

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  lastButtonState = reading;  
  //if (lastButtonState == LOW){
  //  Serial.println("[checkSwitchState] - button low"); 
  //}else if (lastButtonState == HIGH){
  //  Serial.println("[checkSwitchState] - button high");    
  //}

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (lastButtonState != buttonState) {
      buttonState = lastButtonState;

      // toggle the state of the light when the button has been pressed
      if (buttonState == LOW) {
        if ((millis() - lastTransistionTime) > stateTransistionDelay){      
          State ^= true;
          lastTransistionTime = millis();
          Serial.println("[checkSwitchState] - Lamp State Changed");
        }
      }
    }
  }
}

// Setup Method - Runs only once before the main loop. Useful for setting things up
void setup() {
  // Add a short delay on start
  delay(1000);

  // Start Serial
  Serial.begin(115200);
  Serial.println();

  // Check if the flash has been set up correctly
  spiffsCorrectSize = checkFlashConfig();
  if (spiffsCorrectSize) {

    // Setup weak internal pullups
    pinMode(SWITCH_PIN, INPUT_PULLUP);   
    // Init interrupts to allow for switching the light on and off with the button
    //attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), intCallback, FALLING); 
       
    // Init the LED's
    ledStringInit();
    ledModeInit();

    // Get saved settings
    getConfig();

    // Start Wifi
    wifiInit();

    // Setup Webserver
    webServerInit();

    // Setup websockets
    websocketsInit();
  }
  else Serial.println("[setup] -  Flash configuration was not set correctly. Please check your settings under \"tools->flash size:\"");
}

// The Main Loop Methdo - This runs continuously
void loop() {
  // Check if the flash was correctly setup
  if (spiffsCorrectSize) {
    // Handle the captive portal 
    captivePortalDNS.processNextRequest();

    // Handle mDNS 
    MDNS.update();

    // Handle the webserver
    restServer.handleClient();
    
    // Handle Websockets
    webSocket.loop();

    // Get the time when needed
    handleNTP();

    // Update WS clients when needed
    updateClients();

    // Handle the wifi connection 
    handleWifiConnection();

    //Check to see if switch has been pressed to turn light on/off
    //checkSwitchState();     

    // Update the LED's
    handleMode();    

    // Reset the sw watchdog timer
    ESP.wdtFeed();    
  }
  else {
    delay(10000);
    Serial.println("[loop] - Flash configuration was not set correctly. Please check your settings under \"tools->flash size:\"");
  }
}
