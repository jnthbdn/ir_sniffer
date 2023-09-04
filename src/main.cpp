#include <Arduino.h>

constexpr uint16_t RAW_BUFFER_SIZE = 1024;

unsigned long last = 0;
unsigned long raw[RAW_BUFFER_SIZE] = {0};
unsigned raw_idx = 0;

void IRAM_ATTR isr(){
    unsigned long now = micros();

    if( last > 0 ){
        raw[raw_idx++] = now - last;
    }

    last = now;
}

void print_binary(uint8_t byte){
  for(int8_t i = 7; i >= 0; --i){
    Serial.print( (byte & (0x01 << i)) > 0 ? "1" : "0" );
  }
}

void analyse_data(){
  uint8_t octets[27] = {0};
  uint8_t id_bit = 0;
  uint8_t id_octet = 0;

  // On saute les deux première mesure (bit de start)
  for( unsigned i = 2; i < RAW_BUFFER_SIZE; i += 2){

    unsigned long low = raw[i];
    unsigned long high = raw[i+1];

    if( low == 0 || high == 0){
      break;
    }
    else if(high > 3000) {
      // Fin du header
      // Saut des deux prochaines données (bit de start)
      i += 2;
    }
    else if( high > 1000 ){
      // bit '1'
      octets[id_octet] |= 1 << id_bit;
      id_bit++;
    }
    else if( high < 600 ) {
      // bit '0'
      id_bit++;
    }
    else{
      Serial.printf("Erreure de trame position %d, (L: %lu, H: %lu)\r\n", i, low, high);
    }

    if( id_bit >= 8 ){
      id_bit = 0;
      id_octet++;
    }
  }

  for(uint8_t i = 0; i < 27; i++ ){
    print_binary(octets[i]);
    Serial.print(" ");
  }
  Serial.println("");
}

void setup(){
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(D6), isr, CHANGE);
  *((volatile uint32_t*) 0x60000900) &= ~(1); // Hardware WDT OFF
}

void loop(){

    if( Serial.available() > 0 && Serial.read() == 'p' ){
        analyse_data();
        raw_idx = 0;
        last = 0;

        for( unsigned i = 0; i < RAW_BUFFER_SIZE; ++i){
          raw[i] = 0;
        }
    }
}
