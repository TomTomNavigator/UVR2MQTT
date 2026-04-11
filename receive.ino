/*  
 ~~~~~~~
 uvr2web
 ~~~~~~~
 © Elias Kuiter 2013 (http://elias-kuiter.de)
 
 receive.ino:
 Empfangen und Speichern von Datenrahmen der Regelung
 Receive and save data frames by the heating control
 
 */

namespace Receive {

  void start() {
    static bool first_start = true;
    if (first_start) {
      delay(2000);
      first_start = false;
    }
    pulse_count = 0;
    last_bit_change = 0;
    frame_complete = 0;
    attachInterrupt(interrupt, pin_changed, CHANGE);
  }

  void ICACHE_RAM_ATTR stop() {
    detachInterrupt(interrupt);
  }

  //CM -> https://www.reddit.com/r/esp8266/comments/c8lbjr/help_an_idiot_out_trouble_with_interrupts/
  void ICACHE_RAM_ATTR pin_changed() {
    byte val = digitalRead(dataPin); // Zustand einlesen // read state
    unsigned long now = micros();
    unsigned long time_diff = now - last_bit_change;
    last_bit_change = now;
    // einfache Pulsweite? // singe pulse width?
    if (time_diff >= low_width && time_diff <= high_width) {
      process_bit(val);
      return;   
    }
    // doppelte Pulsweite? // double pulse width?
    if (time_diff >= double_low_width && time_diff <= double_high_width) {
      process_bit(!val);
      process_bit(val);
      return;   
    } 
  }

  void ICACHE_RAM_ATTR process_bit(byte b) {
    // den ersten Impuls ignorieren // ignore first pulse
    int count = ++pulse_count;
    if (count & 1)
      return;

    int bit_pos = count / 2;
    int row = bit_pos / 8;
    int col = bit_pos % 8;
    if (b)
      Process::data_bits[row] |= 1 << col; // Bit setzen // set bit
    else
      Process::data_bits[row] &= ~(1 << col); // Bit löschen // clear bit

    if (bit_pos == Process::bit_number) {
      // beende Übertragung, wenn Datenrahmen vollständig
      stop(); // stop receiving when data frame is complete
      frame_complete = 1;
    }
  }

}
