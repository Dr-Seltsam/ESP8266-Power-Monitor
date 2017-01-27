#define BLYNK_MAX_READBYTES 512
/****************************************************************************/
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SimpleTimer.h>
#include "settings.h"
/****************************************************************************/
SimpleTimer timer;
Adafruit_INA219 ina219;
float shuntvoltage = 0.00;
float busvoltage = 0.00;
float current_mA = 0.00, current_mA_Max, current_mA_Avg;
float loadvoltage = 0.00, loadvoltageMax, loadvoltageAvg;
float energy = 0.00, energyPrice = 0.000, energyCost, energyPrevious, energyDifference;
float power = 0.00, powerMax, powerAvg;
int sendTimer, pollingTimer, priceTimer, graphTimer, autoRange, stopwatchResetCounter, counter2, secret, stopwatchTimer;
long stopwatch;
int splitTimer1, splitTimer2, splitTimer3, splitTimer4, splitTimer5;
int sendTimer1, sendTimer2, sendTimer3, sendTimer4, sendTimer5;
float current_AVG, current_AVG_cycle, current_AVG_1, current_AVG_2, current_AVG_3, current_AVG_4, current_AVG_5;
float loadvoltage_AVG, loadvoltage_AVG_cycle, loadvoltage_AVG_1, loadvoltage_AVG_2, loadvoltage_AVG_3, loadvoltage_AVG_4, loadvoltage_AVG_5;
float power_AVG, power_AVG_cycle, power_AVG_1, power_AVG_2, power_AVG_3, power_AVG_4, power_AVG_5;
/****************************************************************************/
void getINA219values() {

  // get the INA219 values and throw some basic math at them
  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  loadvoltage = busvoltage + (shuntvoltage / 1000); // V
  power = (current_mA / 1000) * loadvoltage * 1000; // mW
  energy = energy + (power / 1000 / 1000);

  // nothing connected? set all to 0, otherwise they float around 0.
  if (loadvoltage < 1.1 && loadvoltage > 1 && current_mA < 2 && power < 2) {
    loadvoltage = 0;
    current_mA = 0;
    power = 0;
    energy = 0;
  }

  // gather voltage average from 5 samples over 5 seconds
  loadvoltage_AVG_cycle++;
  if (loadvoltage_AVG_cycle == 1) {
    loadvoltage_AVG_1 = loadvoltage;
  }
  if (loadvoltage_AVG_cycle == 2) {
    loadvoltage_AVG_2 = loadvoltage;
  }
  if (loadvoltage_AVG_cycle == 3) {
    loadvoltage_AVG_3 = loadvoltage;
  }
  if (loadvoltage_AVG_cycle == 4) {
    loadvoltage_AVG_4 = loadvoltage;
  }
  if (loadvoltage_AVG_cycle == 5) {
    loadvoltage_AVG_5 = loadvoltage;
    loadvoltage_AVG_cycle = 0;
  }

  // gather current average from 5 samples over 5 seconds
  current_AVG_cycle++;
  if (current_AVG_cycle == 1) {
    current_AVG_1 = current_mA;
  }
  if (current_AVG_cycle == 2) {
    current_AVG_2 = current_mA;
  }
  if (current_AVG_cycle == 3) {
    current_AVG_3 = current_mA;
  }
  if (current_AVG_cycle == 4) {
    current_AVG_4 = current_mA;
  }
  if (current_AVG_cycle == 5) {
    current_AVG_5 = current_mA;
    current_AVG_cycle = 0;
  }

  // gather power average from 5 samples over 5 seconds
  power_AVG_cycle++;
  if (power_AVG_cycle == 1) {
    power_AVG_1 = power;
  }
  if (power_AVG_cycle == 2) {
    power_AVG_2 = power;
  }
  if (power_AVG_cycle == 3) {
    power_AVG_3 = power;
  }
  if (power_AVG_cycle == 4) {
    power_AVG_4 = power;
  }
  if (power_AVG_cycle == 5) {
    power_AVG_5 = power;
    power_AVG_cycle = 0;
  }

}

// this function is for updaing the REAL TIME values and is on a timer
void sendINA219valuesREAL() {
  // LOAD VOLTAGE (REAL TIME)
  Blynk.virtualWrite(vPIN_VOLTAGE_REAL, String(loadvoltage, 4) + String(" V") );
  // LOAD POWER (REAL TIME)
  if (power > 1000 && autoRange == 1) {
    Blynk.virtualWrite(vPIN_POWER_REAL, String((power / 1000), 3) + String(" W") );
  } else {
    Blynk.virtualWrite(vPIN_POWER_REAL, String(power, 3) + String(" mW") );
  }
  // LOAD CURRENT (REAL TIME)
  if (current_mA > 1000 && autoRange == 1) {
    Blynk.virtualWrite(vPIN_CURRENT_REAL, String((current_mA / 1000), 3) + String(" A") );
  } else {
    Blynk.virtualWrite(vPIN_CURRENT_REAL, String(current_mA, 3) + String(" mA"));
  }
}

// this function is for updaing the AVERGE values and is on a timer
void sendINA219valuesAVG() {
  // LOAD VOLTAGE (AVERAGE)
  loadvoltage_AVG = (loadvoltage_AVG_1 + loadvoltage_AVG_2 + loadvoltage_AVG_3 + loadvoltage_AVG_4 + loadvoltage_AVG_5) / 5;
  Blynk.virtualWrite(vPIN_VOLTAGE_AVG, String(loadvoltage_AVG, 3) + String(" V"));

  // LOAD CURRENT (AVERAGE)
  current_AVG = (current_AVG_1 + current_AVG_2 + current_AVG_3 + current_AVG_4 + current_AVG_5) / 5;
  if (current_AVG > 1000 && autoRange == 1) {
    Blynk.virtualWrite(vPIN_CURRENT_AVG, String((current_AVG / 1000), 2) + String(" A"));
  } else {
    Blynk.virtualWrite(vPIN_CURRENT_AVG, String(current_AVG, 2) + String(" mA"));
  }

  // LOAD POWER (AVERAGE)
  power_AVG = (power_AVG_1 + power_AVG_2 + power_AVG_3 + power_AVG_4 + power_AVG_5) / 5;
  if (power_AVG > 1000 && autoRange == 1) {
    Blynk.virtualWrite(vPIN_POWER_AVG, String((power_AVG / 1000), 2) + String(" W"));
  } else {
    Blynk.virtualWrite(vPIN_POWER_AVG, String(power_AVG, 2) + String(" mW"));
  }

}

// this function is for updaing the MAX values and is on a timer
void sendINA219valuesMAX() {
  // LOAD VOLTAGE (HIGH)
  if (loadvoltage > loadvoltageMax) {
    loadvoltageMax = loadvoltage;
    Blynk.virtualWrite(vPIN_VOLTAGE_PEAK, String(loadvoltageMax, 3) + String(" V") );
  }
  // LOAD CURRENT (HIGH)
  if (current_mA > current_mA_Max) {
    current_mA_Max = current_mA;
    if (current_mA_Max > 1000 && autoRange == 1) {
      Blynk.virtualWrite(vPIN_CURRENT_PEAK, String((current_mA_Max / 1000), 2) + String(" A") );
    } else {
      Blynk.virtualWrite(vPIN_CURRENT_PEAK, String(current_mA_Max, 2) + String(" mA"));
    }
  }
  // LOAD POWER (HIGH)
  if (power > powerMax) {
    powerMax = power;
    if (powerMax > 1000 && autoRange == 1) {
      Blynk.virtualWrite(vPIN_POWER_PEAK, String((powerMax / 1000), 2) + String(" W") );
    } else {
      Blynk.virtualWrite(vPIN_POWER_PEAK, String(powerMax, 2) + String(" mW"));
    }
  }
}

// this function is for updaing the ENERGY values and is on a timer
void sendINA219valuesENERGY() {
  energyDifference = energy - energyPrevious;
  // ENERGY CONSUMPTION
  if (energy > 1000 && autoRange == 1) {
    Blynk.virtualWrite(vPIN_ENERGY_USED, String((energy / 1000), 6) + String(" kWh"));
  } else {
    Blynk.virtualWrite(vPIN_ENERGY_USED, String(energy, 6) + String(" mWh"));
  }
  energyPrevious = energy;
  // ENERGY COST
  energyCost = energyCost + ((energyPrice / 1000 / 100) * energyDifference);
  Blynk.virtualWrite(vPIN_ENERGY_COST, String((energyCost), 8));
}

// this is feeding raw data to the graph
void sendINA219_GraphValues() {
  Blynk.virtualWrite(vPIN_CURRENT_GRAPH, current_AVG);
}

// HOLD BUTTON
BLYNK_WRITE(vPIN_BUTTON_HOLD) {
  if (param.asInt()) {
    timer.disable(sendTimer1);
    timer.disable(sendTimer2);
    timer.disable(sendTimer3);
    timer.disable(sendTimer4);
  } else {
    timer.enable(sendTimer1);
    timer.enable(sendTimer2);
    timer.enable(sendTimer3);
    timer.enable(sendTimer4);
  }
}

// this function only runs when in HOLD mode and select AUTO-RANGE
void updateINA219eXtraValues() {
  Blynk.virtualWrite(vPIN_VOLTAGE_PEAK, String(loadvoltageMax, 3) + String(" V") );
  if (current_AVG > 1000 && autoRange == 1) {
    Blynk.virtualWrite(vPIN_CURRENT_PEAK, String((current_AVG / 1000), 2) + String(" A") );
  } else {
    Blynk.virtualWrite(vPIN_CURRENT_PEAK, String(current_AVG, 2) + String(" mA"));
  }
  if (powerMax > 1000 && autoRange == 1) {
    Blynk.virtualWrite(vPIN_POWER_PEAK, String((powerMax / 1000), 2) + String(" W") );
  } else {
    Blynk.virtualWrite(vPIN_POWER_PEAK, String(powerMax, 2) + String(" mW"));
  }
}

// AUTO RANGE BUTTON
BLYNK_WRITE(vPIN_BUTTON_AUTORANGE) {
  if (param.asInt()) {
    autoRange = 1;
  } else {
    autoRange = 0;
  }
  updateINA219eXtraValues();
}

// RESET AVERAGES
BLYNK_WRITE(vPIN_BUTTON_RESET_AVG) {
  if (param.asInt() && secret == 0) {
    Blynk.virtualWrite(vPIN_VOLTAGE_AVG,  "--- V");
    Blynk.virtualWrite(vPIN_CURRENT_AVG, "--- mA");
    Blynk.virtualWrite(vPIN_POWER_AVG, "--- mW");
    loadvoltage_AVG = loadvoltage;
    loadvoltage_AVG_1 = loadvoltage;
    loadvoltage_AVG_2 = loadvoltage;
    loadvoltage_AVG_3 = loadvoltage;
    loadvoltage_AVG_4 = loadvoltage;
    loadvoltage_AVG_5 = loadvoltage;
    current_AVG = current_mA;
    current_AVG_1 = current_mA;
    current_AVG_2 = current_mA;
    current_AVG_3 = current_mA;
    current_AVG_4 = current_mA;
    current_AVG_5 = current_mA;
    power_AVG = power;
    power_AVG_1 = power;
    power_AVG_2 = power;
    power_AVG_3 = power;
    power_AVG_4 = power;
    power_AVG_5 = power;
    delay(50);
    updateINA219eXtraValues();
  }
}

// RESET PEAKS
BLYNK_WRITE(vPIN_BUTTON_RESET_MAX) {
  if (param.asInt() && secret == 0) {
    Blynk.virtualWrite(vPIN_VOLTAGE_PEAK, "--- V");
    Blynk.virtualWrite(vPIN_CURRENT_PEAK, "--- mA");
    Blynk.virtualWrite(vPIN_POWER_PEAK, "--- mW");
    loadvoltageMax = loadvoltage;
    current_mA_Max = current_mA;
    powerMax = power;
    delay(50);
    updateINA219eXtraValues();
    stopwatchResetCounter = timer.setTimeout(1000, countdownToReset);
  } else {
    timer.disable(stopwatchResetCounter);
  }
}

// this the callback for holding a button longer than 1 second
// it needs work.. secret = 1?? srsly?.. I need to change this
void countdownToReset() {
  secret = 1;
  counter2 = timer.setTimeout(500, countdownToNormal);
  Blynk.virtualWrite(vPIN_ENERGY_TIME, "--:--:--:--");
  stopwatch = 0;
  timer.enable(stopwatchTimer);
}
// callback to change button function back to normal... also needs work
void countdownToNormal() {
  secret = 0;
}

// the stopwatch counter which is run on a timer
void stopwatchCounter() {
  stopwatch++;
  long days = 0;
  long hours = 0;
  long mins = 0;
  long secs = 0;
  String secs_o = ":";
  String mins_o = ":";
  String hours_o = ":";
  secs = stopwatch; //convect milliseconds to seconds
  mins = secs / 60; //convert seconds to minutes
  hours = mins / 60; //convert minutes to hours
  days = hours / 24; //convert hours to days
  secs = secs - (mins * 60); //subtract the coverted seconds to minutes in order to display 59 secs max
  mins = mins - (hours * 60); //subtract the coverted minutes to hours in order to display 59 minutes max
  hours = hours - (days * 24); //subtract the coverted hours to days in order to display 23 hours max
  if (secs < 10) {
    secs_o = ":0";
  }
  if (mins < 10) {
    mins_o = ":0";
  }
  if (hours < 10) {
    hours_o = ":0";
  }
  Blynk.virtualWrite(vPIN_ENERGY_TIME, days + hours_o + hours + mins_o + mins + secs_o + secs);
}

// This section is for setting the kWh price from your electric company.
// I use a SPOT rate which means I need to update it all the time.
// If you know your set price per kWh (in cents), then enter the price at the top of the sketch: FIXED_ENERGY_PRICE
void getPrice() {
  Blynk.virtualWrite(vPIN_ENERGY_API, ENERGY_API); // local API Server to get current power price per mWh
}
BLYNK_WRITE(vPIN_ENERGY_API) {
  energyPrice = param.asFloat();
  Blynk.virtualWrite(vPIN_ENERGY_PRICE, String(energyPrice, 4) + String('c') );
}

// the functions for the split-task timers.
// required to keep the little ESP8266 from disconnections
void splitTask1() {
  sendTimer1 = timer.setInterval(1000, sendINA219valuesREAL);
}
void splitTask2() {
  sendTimer2 = timer.setInterval(1000, sendINA219valuesAVG);
}
void splitTask3() {
  sendTimer3 = timer.setInterval(1000, sendINA219valuesMAX);
}
void splitTask4() {
  sendTimer4 = timer.setInterval(2000, sendINA219valuesENERGY);
}
/****************************************************************************/
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  // CONNECT TO BLYNK
#if defined(USE_LOCAL_SERVER)
  Blynk.begin(AUTH, WIFI_SSID, WIFI_PASS, SERVER);
#else
  Blynk.begin(AUTH, WIFI_SSID, WIFI_PASS, );
#endif
  while (Blynk.connect() == false) {}

  // SETUP OVER THE AIR UPDATES
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.begin();

  // START INA219
  ina219.begin();

  // TIMERS
  pollingTimer = timer.setInterval(1000, getINA219values);
  graphTimer = timer.setInterval(2000, sendINA219_GraphValues);
  stopwatchTimer = timer.setInterval(1000, stopwatchCounter);

  // setup split-task timers so we dont overload ESP
  // with too many virtualWrites per second
  splitTimer1 = timer.setTimeout(200, splitTask1);
  splitTimer2 = timer.setTimeout(400, splitTask2);
  splitTimer3 = timer.setTimeout(600, splitTask3);
  splitTimer4 = timer.setTimeout(800, splitTask4);

  // start in auto-range mode & sync widget to hardware state
  autoRange = 1;
  Blynk.virtualWrite(vPIN_BUTTON_AUTORANGE, 1);

  // Check for fixed energy price and update global 'energyPrice'
#if defined(FIXED_ENERGY_PRICE)
  // else set fixed price with configured price
  energyPrice = FIXED_ENERGY_PRICE;
  Blynk.virtualWrite(vPIN_ENERGY_PRICE, String(FIXED_ENERGY_PRICE, 4) + String('c') );
#else
  // No fixed price set, so pull from local API
  Blynk.virtualWrite(vPIN_ENERGY_API, ENERGY_API);
  priceTimer = timer.setInterval(20000, getPrice); // start a 20sec timer for updates
#endif
}
/****************************************************************************/
void loop() {
  // the loop... dont touch or add to this!
  Blynk.run();
  ArduinoOTA.handle();
  timer.run();
}
