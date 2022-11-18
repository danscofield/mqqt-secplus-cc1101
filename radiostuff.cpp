#include "radiostuff.h"
#include "constants.h"

unsigned int reverse_bits(unsigned int num)
{
  unsigned int NO_OF_BITS = sizeof(num) * 8;
  unsigned int reverse_num = 0;
  int i;
  for (i = 0; i < NO_OF_BITS; i++)
  {
    if ((num & (1 << i)))
      reverse_num |= 1 << ((NO_OF_BITS - 1) - i);
  }
  return reverse_num;
}

int get_payload(unsigned int rolling, unsigned int fixed, uint8_t *dest_buff)
{
  if (fixed >= 0xcfd41b91)
  {
    //printf("Fixed is too big\n");
    return (-1);
  }

  rolling = reverse_bits(rolling & 0xfffffffe);
  uint8_t rolling_base3[20] = {0};
  uint8_t fixed_base3[20] = {0};
  uint8_t code[40] = {0};

  for (int i = 19; i >= 0; i--)
  {
    rolling_base3[i] = rolling % 3;
    rolling = rolling / 3;
    fixed_base3[i] = fixed % 3;
    fixed = fixed / 3;
  }
  int acc = 0;
  int counter = 0;
  for (int i = 0; i < 20; i++)
  {
    if (i == 0 || i == 10)
    {
      acc = 0;
    }
    acc += rolling_base3[i];
    code[counter] = rolling_base3[i];
    acc += fixed_base3[i];
    code[counter + 1] = (acc % 3);
    counter += 2;
  }

  memcpy(dest_buff, code, sizeof(code));
  return (0);
}

int build_secplus_packet(unsigned int rolling, unsigned int fixed, uint8_t *packet_buffer, uint8_t bufsize)
{
  if (bufsize < 226)
  {
    return -1;
  }
  uint8_t blanks[29] = {0};
  memset(blanks, 0x0, sizeof(blanks));

  uint8_t code[40] = {0};
  int counter = 0;

  uint8_t symbols_to_transmit[2 + sizeof(blanks) + sizeof(code) + sizeof(blanks)] = {0};
  memset(symbols_to_transmit, 0x03, sizeof(symbols_to_transmit));

  get_payload(rolling, fixed, code);

  symbols_to_transmit[counter++] = 0;
  for (int i = 0; i < 20; i++)
  {
    symbols_to_transmit[counter++] = code[i];
  }
  counter += sizeof(blanks);
  symbols_to_transmit[counter++] = 2;
  for (int i = 20; i < sizeof(code); i++)
  {
    symbols_to_transmit[counter++] = code[i];
  }

  uint8_t payload[50] = {0};
  memset(&payload, 0x0, sizeof(payload));
  int c = 0;
  for (int i = 1; i < sizeof(symbols_to_transmit); i += 2)
  {
    int low = symbols_to_transmit[i - 1];
    int high = symbols_to_transmit[i];

    if (symbols_to_transmit[i - 1] == 0)
      low = 0b0001;
    if (symbols_to_transmit[i - 1] == 1)
      low = 0b0011;
    if (symbols_to_transmit[i - 1] == 2)
      low = 0b0111;
    if (symbols_to_transmit[i - 1] == 3)
      low = 0b0000;

    if (symbols_to_transmit[i] == 0)
      high = 0b0001;
    if (symbols_to_transmit[i] == 1)
      high = 0b0011;
    if (symbols_to_transmit[i] == 2)
      high = 0b0111;
    if (symbols_to_transmit[i] == 3)
      high = 0b0000;

    int full = (low << 4) | (high);
    payload[c] = full;
    c++;
  }
  uint8_t packet[26 + (4 * sizeof(payload))] = {};
  memset(packet, 0x0, sizeof(packet));
  int offset = 13;
  for (int p = 0; p < 4; p++)
  {
    memcpy(packet + offset, payload, sizeof(payload));
    offset += sizeof(payload);
  }
  memcpy(packet_buffer, packet, sizeof(packet));
  return 0;
}

void configure_radio()
{
  ELECHOUSE_cc1101.setSpiPin(pSCK, pMISO, pMOSI, pCSN);
  if (ELECHOUSE_cc1101.getCC1101())
  { // Check the CC1101 Spi connection.
    Serial.println("Radio Connection OK");
  }
  else
  {
    Serial.println("Radio Connection Error");
  }
  ELECHOUSE_cc1101.Init(); // must be set to initialize the cc1101!

  ELECHOUSE_cc1101.setGDO0(pGDO0);    // set lib internal gdo pin (gdo0). Gdo2 not use for this example.
  ELECHOUSE_cc1101.setCCMode(1);     // set config for internal transmission mode.
  ELECHOUSE_cc1101.setModulation(2); // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
  ELECHOUSE_cc1101.setMHZ(315.0);    // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
  ELECHOUSE_cc1101.setDRate(2);      // Set the Data Rate in kBaud. Value from 0.02 to 1621.83. Default is 99.97 kBaud!
  ELECHOUSE_cc1101.setSyncMode(0);   // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
  ELECHOUSE_cc1101.setAdrChk(0);     // Controls address check configuration of received packages. 0 = No address check. 1 = Address check, no broadcast. 2 = Address check and 0 (0x00) broadcast. 3 = Address check and 0 (0x00) and 255 (0xFF) broadcast.

  ELECHOUSE_cc1101.setPktFormat(0);    // Format of RX and TX data. 0 = Normal mode, use FIFOs for RX and TX. 1 = Synchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins. 2 = Random TX mode; sends random data using PN9 generator. Used for test. Works as normal mode, setting 0 (00), in RX. 3 = Asynchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins.
  ELECHOUSE_cc1101.setLengthConfig(2); // 0 = Fixed packet length mode. 1 = Variable packet length mode. 2 = Infinite packet length mode. 3 = Reserved
  ELECHOUSE_cc1101.setCrc(0);          // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.
  ELECHOUSE_cc1101.setFEC(0);          // Enable Forward Error Correction (FEC) with interleaving for packet payload (Only supported for fixed packet length mode. 0 = Disable. 1 = Enable.
}
void send_packet(uint8_t* secplus_buffer, uint8_t buf_size) {
    ELECHOUSE_cc1101.SendData(secplus_buffer, buf_size);
}