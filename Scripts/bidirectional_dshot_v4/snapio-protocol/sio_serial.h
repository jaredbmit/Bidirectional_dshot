/**
 * @file sio_serial.h
 * @brief Serial protocol for snap-stack io board
 * @author Parker Lusk <plusk@mit.edu>
 * @date 14 Apr 2021
 */

#ifndef SIO_SERIAL_H_
#define SIO_SERIAL_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

//=============================================================================
// message types
//=============================================================================

typedef enum {
  SIO_SERIAL_MSG_MOTORCMD,
  SIO_SERIAL_NUM_MSGS
} sio_msg_type_t;

//=============================================================================
// message definitions
//=============================================================================

typedef struct {
  uint16_t usec[6]; // per motor pwm cmd in microseconds
} sio_serial_motorcmd_msg_t;

// payload lengths
static const float SIO_SERIAL_PAYLOAD_LEN[] = {
  sizeof(sio_serial_motorcmd_msg_t),
};

// manually indicate the largest msg payload
static const size_t SIO_SERIAL_MAX_PAYLOAD_LEN = sizeof(sio_serial_motorcmd_msg_t);

//=============================================================================
// generic message type
//=============================================================================

static const uint8_t SIO_SERIAL_MAGIC = 0xA5;

typedef struct __attribute__((__packed__)) {
  uint8_t magic;
  uint8_t type;
  uint8_t payload[SIO_SERIAL_MAX_PAYLOAD_LEN];
  uint8_t crc;
} sio_serial_message_t;

static const size_t SIO_SERIAL_MAX_MESSAGE_LEN = sizeof(sio_serial_message_t);

//=============================================================================
// utility functions
//=============================================================================

// source: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html#gab27eaaef6d7fd096bd7d57bf3f9ba083
static inline uint8_t sio_serial_update_crc(uint8_t inCrc, uint8_t inData)
{
  uint8_t i;
  uint8_t data;

  data = inCrc ^ inData;

  for ( i = 0; i < 8; i++ )
  {
    if (( data & 0x80 ) != 0 )
    {
      data <<= 1;
      data ^= 0x07;
    }
    else
    {
      data <<= 1;
    }
  }
  return data;
}

static inline void sio_serial_finalize_message(sio_serial_message_t *msg)
{
  msg->magic = SIO_SERIAL_MAGIC;

  uint8_t crc = 0;
  crc = sio_serial_update_crc(crc, msg->magic);
  crc = sio_serial_update_crc(crc, msg->type);
  for (size_t i=0; i<SIO_SERIAL_PAYLOAD_LEN[msg->type]; ++i)
  {
    crc = sio_serial_update_crc(crc, msg->payload[i]);
  }

  msg->crc = crc;
}

static inline size_t sio_serial_send_to_buffer(uint8_t *dst, const sio_serial_message_t *src)
{
  size_t offset = 0;
  memcpy(dst + offset, &src->magic,  sizeof(src->magic)); offset += sizeof(src->magic);
  memcpy(dst + offset, &src->type,   sizeof(src->type));  offset += sizeof(src->type);
  memcpy(dst + offset, src->payload, SIO_SERIAL_PAYLOAD_LEN[src->type]); offset += SIO_SERIAL_PAYLOAD_LEN[src->type];
  memcpy(dst + offset, &src->crc,    sizeof(src->crc)); offset += sizeof(src->crc);
  return offset;
}

//=============================================================================
// Motor command message
//=============================================================================

static inline void sio_serial_motorcmd_msg_pack(sio_serial_message_t *dst, const sio_serial_motorcmd_msg_t *src)
{
  dst->type = SIO_SERIAL_MSG_MOTORCMD;
  size_t offset = 0;
  memcpy(dst->payload + offset, &src->usec, sizeof(src->usec)); offset += sizeof(src->usec);
  sio_serial_finalize_message(dst);
}

static inline void sio_serial_motorcmd_msg_unpack(sio_serial_motorcmd_msg_t *dst, const sio_serial_message_t *src)
{
  size_t offset = 0;
  memcpy(&dst->usec, src->payload + offset, sizeof(dst->usec)); offset += sizeof(dst->usec);
}

static inline size_t sio_serial_motorcmd_msg_send_to_buffer(uint8_t *dst, const sio_serial_motorcmd_msg_t *src)
{
  sio_serial_message_t msg;
  sio_serial_motorcmd_msg_pack(&msg, src);
  return sio_serial_send_to_buffer(dst, &msg);
}

//==============================================================================
// parser
//==============================================================================

typedef enum
{
  SIO_SERIAL_PARSE_STATE_IDLE,
  SIO_SERIAL_PARSE_STATE_GOT_MAGIC,
  SIO_SERIAL_PARSE_STATE_GOT_TYPE,
  SIO_SERIAL_PARSE_STATE_GOT_PAYLOAD
} sio_serial_parse_state_t;

static inline bool sio_serial_parse_byte(uint8_t byte, sio_serial_message_t *msg)
{
  static sio_serial_parse_state_t parse_state = SIO_SERIAL_PARSE_STATE_IDLE;
  static uint8_t crc_value = 0;
  static size_t payload_index = 0;
  static sio_serial_message_t msg_buffer;

  bool got_message = false;
  switch (parse_state)
  {
  case SIO_SERIAL_PARSE_STATE_IDLE:
    if (byte == SIO_SERIAL_MAGIC)
    {
      crc_value = 0;
      payload_index = 0;

      msg_buffer.magic = byte;
      crc_value = sio_serial_update_crc(crc_value, byte);

      parse_state = SIO_SERIAL_PARSE_STATE_GOT_MAGIC;
    }
    break;

  case SIO_SERIAL_PARSE_STATE_GOT_MAGIC:
    msg_buffer.type = byte;
    crc_value = sio_serial_update_crc(crc_value, byte);
    parse_state = SIO_SERIAL_PARSE_STATE_GOT_TYPE;
    break;

  case SIO_SERIAL_PARSE_STATE_GOT_TYPE:
    msg_buffer.payload[payload_index++] = byte;
    crc_value = sio_serial_update_crc(crc_value, byte);
    if (payload_index == SIO_SERIAL_PAYLOAD_LEN[msg_buffer.type])
    {
      parse_state = SIO_SERIAL_PARSE_STATE_GOT_PAYLOAD;
    }
    break;

  case SIO_SERIAL_PARSE_STATE_GOT_PAYLOAD:
    msg_buffer.crc = byte;
    if (msg_buffer.crc == crc_value)
    {
      got_message = true;
      memcpy(msg, &msg_buffer, sizeof(msg_buffer));
    }
    parse_state = SIO_SERIAL_PARSE_STATE_IDLE;
    break;
  }

  return got_message;
}

#endif // SIO_SERIAL_H_
