/*  
 ~~~~~~~
 uvr2web
 ~~~~~~~
 © Elias Kuiter 2013 (http://elias-kuiter.de)
 
 receive.h:
 Empfangen und Speichern von Datenrahmen der Regelung
 Receive and save data frames by the heating control
 
 */

namespace Receive {
  
  // Dekodierung des Manchester-Codes // decoding the manchester code

  // Pulsweite bei 488hz: 1000ms/488 = 2,048ms = 2048µs
  // 2048µs / 2 = 1024µs (2 Pulse für ein Bit)
  // pulse width at 488hz: 1000ms/488 = 2,048ms = 2048µs
  // 2048µs / 2 = 1024µs (2 pulses for one bit)
  const unsigned long pulse_width = 1024; // µs
  // % Toleranz für Abweichungen bei der Pulsweite
  const int percentage_variance = 10; // % tolerance for variances at the pulse width
  // 1001 oder 0110 sind zwei aufeinanderfolgende Pulse ohne Übergang
  // 1001 or 0110 are two sequential pulses without transition
  const unsigned long double_pulse_width = pulse_width * 2;
  // Berechnung der Toleranzgrenzen für Abweichungen
  // calculating the tolerance limits for variances
  const unsigned long low_width = pulse_width - (pulse_width *  percentage_variance / 100);
  const unsigned long high_width = pulse_width + (pulse_width * percentage_variance / 100);
  const unsigned long double_low_width = double_pulse_width - (pulse_width * percentage_variance / 100);
  const unsigned long double_high_width = double_pulse_width + (pulse_width * percentage_variance / 100);
  volatile unsigned long last_bit_change = 0; // Merken des letzten Übergangs // remember the last transition
  volatile int pulse_count; // Anzahl der empfangenen Pulse // number of received pulses
#define BIT_COUNT (pulse_count / 2)
  volatile byte frame_complete = 0; // full fresh frame received and ready for processing
  
  void start(); // Übertragung beginnen // start receiving
  void ICACHE_RAM_ATTR stop(); // Übertragung stoppen // stop receiving
  // wird aufgerufen, sobald sich der Zustand am Daten-Pin ändert
  void ICACHE_RAM_ATTR pin_changed(); // is called when the state of the data pin changes
  // speichert ein von pin_changed ermitteltes Bit
  void ICACHE_RAM_ATTR process_bit(unsigned char b); // saves a bit detected by pin_changed

}
