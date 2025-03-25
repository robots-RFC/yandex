#include <rdm6300.h>
#define RFID_PIN      4

Rdm6300 rdm6300;

void setup() {
  rdm6300.begin(RFID_PIN);
  Serial.begin(115200);
}

void loop() {
  uint16_t card_id = rdm6300.get_tag_id();
  Serial.println(card_id);

}
