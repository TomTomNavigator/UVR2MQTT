namespace Process {

  // Variable definitions
  const int bit_number = (64 * (8 + 1 + 1) + 16) * 2 + additionalBits;
  byte data_bits[bit_number / 8 + 1]; // jedes Bit wird in eine Bitmap einsortiert // every bit gets sorted into a bitmap
  int start_bit; // erstes Bit des Datenrahmens // first bit of data frame
  sensor_t sensor;

  void start() {
    if (prepare()) {
      Dump::start();
    }
  }

  boolean prepare() {
    start_bit = analyze();
    if (start_bit == -1) {
      invert();
      start_bit = analyze();
    }
    trim();
    return check_device();
  } 

  int analyze() {
    byte sync = 0; // Initialize to 0
    for (int i = 0; i < bit_number; i++) {
      if (read_bit(i))
        sync++;
      else
        sync = 0;
      if (sync == 16) {
        while (read_bit(i) == 1)
          i++;
        return i;
      }
    }
    return -1;
  }

  void invert() {
    for (int i = 0; i < read_bit(i); i++)
      write_bit(i, read_bit(i) ? 0 : 1);
  }

  byte read_bit(int pos) {
    int row = pos / 8;
    int col = pos % 8;
    return (((data_bits[row]) >> (col)) & 0x01);
  }
  
  void write_bit(int pos, byte set) {
    int row = pos / 8;
    int col = pos % 8;
    if (set)
      data_bits[row] |= 1 << col;
    else
      data_bits[row] &= ~(1 << col);
  }

  void trim() {    
    for (int i = start_bit, bit = 0; i < bit_number; i++) {
      int offset = i - start_bit;
      if (offset % 10 && (offset + 1) % 10) {
        write_bit(bit, read_bit(i));
        bit++;
      }
    }
  }

  boolean check_device() {
    if (data_bits[0] == 0x80 && data_bits[1] == 0x7f)
      return true;
    else
      return false;
  }

  void fetch_sensor(int number) {
    sensor.number = number;
    sensor.invalid = false;
    sensor.mode = -1;
    float value = 0.0; // Initialize to 0.0
    number = 6 + number * 2;
    byte sensor_low = data_bits[number];
    byte sensor_high = data_bits[number + 1];
    number = sensor_high << 8 | sensor_low;
    sensor.type = (number & 0x7000) >> 12;
    if (!(number & 0x8000)) {
      number &= 0xfff;
      switch (sensor.type) {
      case DIGITAL:
        value = false;
        break;
      case TEMP:
        value = number * 0.1;
        break;
      case RAYS:
        value = number;
        break;
      case VOLUME_FLOW:
        value = number * 4;
        break;
      case ROOM:
        sensor.mode = (number & 0x600) >> 9;
        value = (number & 0x1ff) * 0.1;
        break;
      default:
        sensor.invalid = true;
      }
    } else {
      number |= 0xf000;
      switch (sensor.type) {
      case DIGITAL:
        value = true;
        break;
      case TEMP:
        value = (number - 65536) * 0.1;
        break;
      case RAYS:
        value = number - 65536;
        break;
      case VOLUME_FLOW:
        value = (number - 65536) * 4;
        break;
      case ROOM:
        sensor.mode = (number & 0x600) >> 9;
        value = ((number & 0x1ff) - 65536) * 0.1;
        break;
      default:
        sensor.invalid = true;
      }
    }
    sensor.value = value;
  }

  boolean fetch_output(int output) {
    int outputs = data_bits[41] * 256 + data_bits[40];
    return !!(outputs & (1 << (output - 1)));
  }
}
