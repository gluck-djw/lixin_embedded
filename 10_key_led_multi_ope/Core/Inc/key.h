typedef enum {
  KEY_STATE_IDLE = 0,
  KEY_STATE_PRESSED = 1,
  // KEY_STATE_WAIT_DOUBLE = 2,

} key_state_t;

typedef enum {
  KEY_NONE_PRESSED = 0,
  KEY_SHORT_PRESSED = 1,
  KEY_LONG_PRESSED = 2,
  // KEY_DOUBLE_PRESSED = 4,
} key_event_t;

key_event_t key_tick(void);