// Copyright (C) 2012 Mark R. Stevens
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// Code derived from:
//   https://github.com/stevemarple/MicroMag/blob/master/MicroMag.cpp
//
#include <sensors/magnetometer/Magnetometer.h>

#include <Arduino.h>

#include <vendor/Spi.h>

#include <math.h>

#define MM_PERIOD_32 0
#define MM_PERIOD_64 1
#define MM_PERIOD_128 2
#define MM_PERIOD_256 3
#define MM_PERIOD_512 4
#define MM_PERIOD_1024 5
#define MM_PERIOD_2048 6
#define MM_PERIOD_4096 7

namespace ardadv
{
  namespace sensors
  {
    namespace magnetometer
    {
      Magnetometer::Magnetometer()
      : mValueX(0.0f)
      , mValueY(0.0f)
      , mValueZ(0.0f)
      , mValid(false)
      {
      }
      bool Magnetometer::setup(const DRDY& drdy, const RESET& reset, const CS& ssnot)
      {

        // Set up the spi interface
        //
        SPI.setClockDivider(SPI_CLOCK_DIV32);
        SPI.setDataMode(SPI_MODE0);
        SPI.setBitOrder(MSBFIRST);

        // Store the pins and their modes
        //
        mDataReady.reset(drdy,   INPUT);
        mReset.reset(reset, OUTPUT);
        mChipSelect.reset(ssnot, OUTPUT);

        // Give the pins initial values
        //
        mDataReady.digitalWrite(HIGH);
        mReset.digitalWrite(LOW);
        mChipSelect.digitalWrite(HIGH);

        // Make one reading to switch device into low power mode
        //
        int16_t tmp = 0;
        return read(0, MM_PERIOD_32, tmp, 0);

      }
      bool Magnetometer::convert(uint8_t axis, uint8_t period) const
      {

        // Check if the period is valid
        //
        if (period > MM_PERIOD_4096)
        {
          return false;
        }

        // Build the command
        //
        uint8_t cmd = 0;
        cmd |= (axis + 1);
        cmd |= (period << 4);

        // Select the device (using the default SPI pins)
        //
        mChipSelect.digitalWrite(LOW);

        // Pulse reset
        //
        pulseReset();

        // Send the command byte
        //
        SPI.transfer(cmd);

        // Done
        //
        return true;
      }
      int16_t Magnetometer::getResult() const
      {
        // Select the device (using the default SPI pins)
        //
        mChipSelect.digitalWrite(LOW);

        // Read 2 bytes
        //
        const int16_t r0 = SPI.transfer(0);
        const int16_t r1 = SPI.transfer(0);

        // De-select the device (using the default SPI pins)
        //
        mChipSelect.digitalWrite(HIGH);

        // Return result as a 16 bit number
        //
        return (r0 << 8) | r1;
      }
      bool Magnetometer::read(uint8_t axis, uint8_t period, int16_t& result, uint16_t timeout) const
      {
        // Issue the read command for the requested axis
        //
        if (! convert(axis, period))
        {
          return false;
        }

        // Wait for ready signal
        //
        // Set a default timeout which is appropriate for the selected
        // period. See data sheet for details. Values used are 1us larger
        // to account for +/-1 jitter.
        //
        if (timeout == 0)
        {
          switch (period)
          {
            case MM_PERIOD_32:
              timeout = 501;
              break;
            case MM_PERIOD_64:
              timeout = 1001;
              break;
            case MM_PERIOD_128:
              timeout = 2001;
              break;
            case MM_PERIOD_256:
              timeout = 4001;
              break;
            case MM_PERIOD_512:
              timeout = 7501;
              break;
            case MM_PERIOD_1024:
              timeout = 15001;
              break;
            case MM_PERIOD_2048:
              timeout = 35501;
              break;
            case MM_PERIOD_4096:
              timeout = 60001;
              break;
            default:
              return false;
          }
        }

        // Wait until device reports it is ready, or timeout is reached
        //
        const unsigned long t = micros();
        while (!mDataReady.digitalRead())
        {
          if (micros() - t > timeout)
          {
            return false;
          }
        }

        // Get the result
        //
        result = getResult();

        // Done
        //
        return true;
      }
      void Magnetometer::pulseReset() const
      {
        mReset.digitalWrite(HIGH);
        ::delayMicroseconds(1);
        mReset.digitalWrite(LOW);
      }
      float Magnetometer::readAxis(int axis)
      {

        // Read
        //
        int16_t result = 0;
        if (! read(axis, MM_PERIOD_32, result, 0))
        {
          mValid = false;
          return 0.0f;
        }

        // Return the result
        //
        return result;
      }
      void Magnetometer::update()
      {

        // Basic operation will follow these steps. Refer to the timing
        // diagrams on the following page.
        //
        // 1. CS is brought low.
        //
        // 2. Pulse RESET high (return to low state). You must RESET the
        //    MicroMag3 before every measurement.
        //
        // 3. Data is clocked in on the MOSI line. Once eight bits are read
        //    in, the MicroMag3 will execute the command.
        //
        // 4. The MicroMag3 will make the measurement. A measurement consists
        //    of forward biasing the sensor and making a period count; then
        //    reverse biasing the sensor and counting again; and finally,
        //    taking the difference between the two bias directions.
        //
        // 5. At the end of the measurement, the DRDY line is set to high
        //    indicating that the data is ready. In response to the next
        //    16 SCLK pulses, data is shifted out on the MISO line.
        //
        // If you need to make another measurement, go to Step 2. You can
        // send another command after the reset. In this case, keep CS
        // low. If you will not be using the MicroMag3, set CS to high to
        // disable the SPI port.

        mValid  = true;

        mChipSelect.digitalWrite(LOW);
        ::delay(2);

        SPI.setClockDivider(SPI_CLOCK_DIV32);
        SPI.setDataMode(SPI_MODE0);
        SPI.setBitOrder(MSBFIRST);

        mValueX = readAxis(0);
        mValueY = readAxis(1);
        mValueZ = readAxis(2);

        mChipSelect.digitalWrite(HIGH);

      }
    }
  }
}

