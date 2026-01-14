/*  
 ~~~~~~~
 uvr2web
 ~~~~~~~
 © Elias Kuiter 2013 (http://elias-kuiter.de)
 
 dump.ino:
 Ausgabe der Daten auf der seriellen Schnittstelle
 Data output via serial interface
 
 */

namespace Dump {

  void start() {
    sensors();
    outputs();
  }

  void sensors() {
    for (int i = 1; i <= 6; i++) {
      Process::fetch_sensor(i);
      sensor();
    }
  }

  void sensor() {
    int sNr = Process::sensor.number;
    if ((Process::sensor.value <1200) && (Process::sensor.value > -40)) {
      // Convert float to C-style string. (width 4, precision 2)
      dtostrf(Process::sensor.value, 4, 2, SensorValue[sNr]);
    }
  }

  void outputs() {
    for (int i = 1; i <= 6; i++) {
      Ausgang[i]= Process::fetch_output(i);
    }
  }
}
