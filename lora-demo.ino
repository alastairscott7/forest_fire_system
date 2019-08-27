#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <DHT.h>

#define DHTPIN 9
#define DHTTYPE DHT11

//DHT object
DHT dht(DHTPIN, DHTTYPE);

// LoRaWAN NwkSKey - Network session key
static const PROGMEM u1_t NWKSKEY[16] = { 0xA0, 0x6E, 0xFA, 0xF4, 0xBC, 0x72, 0xAC, 0x99, 0xF5, 0x4D, 0xB3, 0x82, 0xFA, 0x4E, 0x23, 0x03 };

// LoRaWAN AppSKey - Application session key
static const u1_t PROGMEM APPSKEY[16] = { 0xBD, 0xBF, 0x90, 0x68, 0x53, 0x20, 0xBC, 0x8B, 0xD1, 0x24, 0xF6, 0x31, 0xF6, 0xA3, 0x66, 0xA2 };

// LoRaWAN end-device address (DevAddr)
static const u4_t DEVADDR = 0x26021797; // <-- Change this address for every node!

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty cycle limitations).
const unsigned TX_INTERVAL = 7;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 8,  
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 4,
  .dio = {3,6,11},
};

int LED;

void onEvent (ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  
  switch(ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
      
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
      
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
      
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
      
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
      
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      break;
      
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
      
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
      
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
      
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      
      if (LMIC.txrxFlags & TXRX_ACK) Serial.println(F("Received ack"));
      
      if (LMIC.dataLen) {
        Serial.println("Received " + String(LMIC.dataLen) + " bytes of payload");
        for (int i = 0; i < LMIC.dataLen; i++) {
              if (LMIC.frame[LMIC.dataBeg + i] < 0x10) {
                  Serial.print(F("0.."));
              }
              Serial.print(" : " + String(LMIC.frame[LMIC.dataBeg + i]));
          }
          Serial.println();
          if (LMIC.frame[LMIC.dataBeg] > 0){
            Serial.println("Switch light on " + String(LMIC.frame[LMIC.dataBeg]));
            digitalWrite(LED_BUILTIN, HIGH);
          }else {
            Serial.println("Switch off light " + String(LMIC.frame[LMIC.dataBeg]));
            digitalWrite(LED_BUILTIN, LOW);
          }
        }

      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
      break;
    
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;

    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;

    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;

    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

void do_send(osjob_t* j) {
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
        
    } else {
      
      int16_t h = dht.readHumidity() * 100;
      // Read temperature as Celsius (the default)
      int16_t t = dht.readTemperature() * 100;
      // max uint = 65535

      int16_t lad = 49;
      int16_t lam = .38077 * 1000;
      int16_t lod = 123;
      int16_t lom = .25332 * 1000;
      int16_t s = 0;
      int16_t w = 1;
      int16_t a = 1200 * 100;

      byte myData[15];
      myData[0] = highByte(t);
      myData[1] = lowByte(t);
      myData[2] = highByte(h);
      myData[3] = lowByte(h);  
      
      myData[4] = highByte(lad);
      myData[5] = lowByte(lad); 
      myData[6] = highByte(lam);
      myData[7] = lowByte(lam);   
      myData[8] = lowByte(s);    
      
      myData[9] = highByte(lod);
      myData[10] = lowByte(lod); 
      myData[11] = highByte(lom);
      myData[12] = lowByte(lom);      
      myData[13] = lowByte(w);   
          
      myData[14] = highByte(a);     
      myData[15] = lowByte(a);   

      myData[16] = lowByte(255);

      String strData;
      for (char c : myData) strData += c;
      Serial.println("Humidity: " + String(h) + ", Temp: " + String(t) + ", lat " + String(lad) + ", Lon: " + String(lod) + " Payload: " + strData);
      
      // Prepare upstream data transmission at the next possible time.

      LMIC_setTxData2(1, myData, sizeof(myData), 0);    
      Serial.println(F("Packet queued"));
    }    
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  //digitalWrite(LED_BUILTIN, HIGH);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  dht.begin();

  /*
   * Getting Started with Adafruit Feather M0 LoRa
   * https://startiot.telenor.com/learning/getting-started-with-adafruit-feather-m0-lora/
   * 
   * How to measure fine dust via LoRaWan based on adafruit feather m0 lora boards and sds011/sds018 sensors
   * https://github.com/marcuscbehrens/loralife
   * 
   */

 // Serial.println(F("Starting"));

  #ifdef VCC_ENABLE
  // For Pinoccio Scout boards
  pinMode(VCC_ENABLE, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(VCC_ENABLE, HIGH);
  delay(1000);
  #endif

  // LMIC init
  os_init();
  
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

  // Set static session parameters. Instead of dynamically establishing a session
  // by joining the network, precomputed session parameters are be provided.

  #ifdef PROGMEM
  // On AVR, these values are stored in flash and only copied to RAM
  // once. Copy them to a temporary buffer here, LMIC_setSession will
  // copy them into a buffer of its own again.
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
  #else
  // If not running an AVR with PROGMEM, just use the arrays directly
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
  #endif

  #if defined(CFG_eu868)
  // Set up the channels used by the Things Network, which corresponds
  // to the defaults of most gateways. Without this, only three base
  // channels from the LoRaWAN specification are used, which certainly
  // works, so it is good for debugging, but can overload those
  // frequencies, so be sure to configure the full frequency range of
  // your network here (unless your network autoconfigures them).
  // Setting up channels should happen after LMIC_setSession, as that
  // configures the minimal channel set.
  // NA-US channels 0-71 are configured automatically
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
  // TTN defines an additional channel at 869.525Mhz using SF9 for class B
  // devices' ping slots. LMIC does not have an easy way to define set this
  // frequency and support for class B is spotty and untested, so this
  // frequency is not configured here.
  #elif defined(CFG_us915)
  // NA-US channels 0-71 are configured automatically
  // but only one group of 8 should (a subband) should be active
  // TTN recommends the second sub band, 1 in a zero based count.
  // https://github.com/TheThingsNetwork/gateway-conf/blob/master/US-global_conf.json
  LMIC_selectSubBand(1);
  #endif

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF7,14);

  // Start job
  do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}
