const uint8_t LETTER_A = SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G;
const uint8_t LETTER_B = SEG_C | SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t LETTER_C = SEG_A | SEG_D | SEG_E | SEG_F;
const uint8_t LETTER_D = SEG_B | SEG_C | SEG_D | SEG_E | SEG_G;
const uint8_t LETTER_E = SEG_A | SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t LETTER_F = SEG_A | SEG_E | SEG_F | SEG_G;

const uint8_t LETTER_I = SEG_E | SEG_F;
const uint8_t LETTER_L = SEG_D | SEG_F | SEG_E;

const uint8_t LETTER_n = SEG_C | SEG_E | SEG_G;
const uint8_t LETTER_O = SEG_C | SEG_D | SEG_E | SEG_G;

const uint8_t LETTER_r = SEG_E | SEG_G;

const uint8_t LETTER_t = SEG_D | SEG_E | SEG_F | SEG_G ; //kind of

const uint8_t SEG_CONF[] = {
  LETTER_C,                                        // C
  LETTER_O,                                        // o
  LETTER_n,                                        // n
  LETTER_F                                         // F
};

const uint8_t SEG_BOOT[] = {
  LETTER_B,                                        // b
  LETTER_O,                                        // o
  LETTER_O,                                        // o
  LETTER_t                                         // t - kinda
};

const uint8_t SEG_ERR[] = {
  LETTER_E,                                       // E
  LETTER_r,                                       // r
  LETTER_r,                                       // r
  0
  };

const uint8_t SEG_FILE[] = {
  LETTER_F,                                        // F
  LETTER_I,                                        // I
  LETTER_L,                                        // L
  LETTER_E                                         // E
};
