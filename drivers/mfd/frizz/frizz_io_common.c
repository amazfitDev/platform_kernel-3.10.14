#include "frizz_io_common.h"

void prepare_command(unsigned char command, unsigned int reg_addr, unsigned char *tx_buff) {
  tx_buff[0] = command;
  tx_buff[1] = (unsigned char)reg_addr;
}

void prepare_data(unsigned int data, unsigned char *tx_buff) {
  tx_buff[2] = (data >> 24) & 0xff;
  tx_buff[3] = (data >> 16) & 0xff;
  tx_buff[4] = (data >>  8) & 0xff;
  tx_buff[5] = (data >>  0) & 0xff;
}

