/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#if !defined(PIN_LED)
#define PIN_LED     13
#endif

#define PIN_DSTAR   2
#define PIN_DMR     3 
#define PIN_YSF     4
#define PIN_P25     5

#define PIN_TX      6

#define PIN_LOCKOUT 7

void setup()
{
  Serial.begin(115200);

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega32U4__) || defined(__SAM3X8E__)
  Serial1.begin(9600);
#endif

  pinMode(PIN_LED,     OUTPUT);
  pinMode(PIN_DSTAR,   OUTPUT);
  pinMode(PIN_DMR,     OUTPUT);
  pinMode(PIN_YSF,     OUTPUT);
  pinMode(PIN_P25,     OUTPUT);
  pinMode(PIN_TX,      OUTPUT);
  pinMode(PIN_LOCKOUT, INPUT);

  digitalWrite(PIN_DSTAR, LOW);
  digitalWrite(PIN_DMR,   LOW);
  digitalWrite(PIN_YSF,   LOW);
  digitalWrite(PIN_P25,   LOW);
  digitalWrite(PIN_TX,    LOW);
}

#define UMP_FRAME_START   0xF0U

#define UMP_HELLO         0x00U

#define UMP_SET_MODE      0x01U
#define UMP_SET_TX        0x02U

#define UMP_WRITE_SERIAL  0x10U

#define UMP_STATUS        0x50U

#define MODE_IDLE   0U
#define MODE_DSTAR  1U
#define MODE_DMR    2U
#define MODE_YSF    3U
#define MODE_P25    4U

bool     m_started = false;
uint32_t m_count   = 0U;
bool     m_led     = false;

uint8_t m_buffer[256U];
uint8_t m_offset = 0U;
uint8_t m_length = 0U;

bool    m_lockout = false;

void loop()
{
  while (Serial.available()) {
    uint8_t c = Serial.read();  

    if (m_offset == 0U) {
      if (c == UMP_FRAME_START) {
        m_buffer[m_offset] = c;
        m_offset = 1U;
      }
    } else if (m_offset == 1U) {
      m_length = m_buffer[m_offset] = c;
      m_offset = 2U;
    } else {
      m_buffer[m_offset] = c;
      m_offset++;

      if (m_length == m_offset) {
        switch (m_buffer[2U]) {
        case UMP_HELLO:
          m_started = true;
          break;
        case UMP_SET_MODE:
          digitalWrite(PIN_DSTAR, m_buffer[3U] == MODE_DSTAR ? HIGH : LOW);
          digitalWrite(PIN_DMR,   m_buffer[3U] == MODE_DMR   ? HIGH : LOW);
          digitalWrite(PIN_YSF,   m_buffer[3U] == MODE_YSF   ? HIGH : LOW);
          digitalWrite(PIN_P25,   m_buffer[3U] == MODE_P25   ? HIGH : LOW);
          break;
        case UMP_SET_TX:
          digitalWrite(PIN_TX, m_buffer[3U] == 0x01U ? HIGH : LOW);
          break;
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega32U4__) || defined(__SAM3X8E__)
        case UMP_WRITE_SERIAL:
          Serial1.write(m_buffer + 3U, m_length - 3U);
          break;
#endif
        default:
          break;
        }

        m_length = 0U;
        m_offset = 0U;
      }
    }
  }

  bool lockout = digitalRead(PIN_LOCKOUT) == HIGH;
  if (lockout != m_lockout) {
    uint8_t data[4U];
    data[0U] = UMP_FRAME_START;
    data[1U] = 4U;
    data[2U] = UMP_STATUS;
    data[3U] = lockout ? 0x01U : 0x00U;
    Serial.write(data, 4U);

    m_lockout = lockout;
  }

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega32U4__) || defined(__SAM3X8E__)
  while (Serial1.available())
    Serial1.read();
#endif

  m_count++;
  if (m_started) {
    if (m_count > 3200U) {
      digitalWrite(PIN_LED, m_led ? LOW : HIGH);
      m_led = !m_led;
      m_count = 0U;
    }
  } else {
    if (m_count > 32000U) {
      digitalWrite(PIN_LED, m_led ? LOW : HIGH);
      m_led = !m_led;
      m_count = 0U;
    }
  }
}

