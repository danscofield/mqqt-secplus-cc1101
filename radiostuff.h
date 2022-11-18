#include <ELECHOUSE_CC1101_SRC_DRV.h>

unsigned int reverse_bits(unsigned int num);
int get_payload(unsigned int rolling, unsigned int fixed, uint8_t* dest_buff);
int build_secplus_packet(unsigned int rolling, unsigned int fixed, uint8_t* packet_buffer, uint8_t bufsize);
void configure_radio();
void send_packet(uint8_t* secplus_buffer, uint8_t buf_size);
