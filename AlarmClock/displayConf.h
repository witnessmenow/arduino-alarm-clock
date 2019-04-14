const uint8_t LETTER_A = SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G;
const uint8_t LETTER_B = SEG_C | SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t LETTER_C = SEG_A | SEG_D | SEG_E | SEG_F;
const uint8_t LETTER_D = SEG_B | SEG_C | SEG_D | SEG_E | SEG_G;
const uint8_t LETTER_E = SEG_A | SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t LETTER_F = SEG_A | SEG_E | SEG_F | SEG_G;

const uint8_t LETTER_O = SEG_C | SEG_D | SEG_E | SEG_G;

const uint8_t SEG_CONF[] = {
  LETTER_C,                                        // C
  LETTER_O,                                        // o
  SEG_C | SEG_E | SEG_G,                           // n
  LETTER_F                                         // F
};

const uint8_t SEG_BOOT[] = {
  LETTER_B,                                        // b
  LETTER_O,                                        // o
  LETTER_O,                                        // o
  SEG_D | SEG_E | SEG_F | SEG_G                    // t - kinda
};
