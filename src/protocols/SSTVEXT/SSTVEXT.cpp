#include "SSTVEXT.h"
#if !RADIOLIB_EXCLUDE_SSTVEXT

const SSTVEXTMode_t Robot36 {
  .visCode = RADIOLIB_SSTVEXT_ROBOT_36,
  .width = 320,
  .height = 240,
  .scanPixelLen = 2750,
  .numTones = 4,
  .tones = {
    { .type = toneext_t::GENERIC,      .len = 9000,  .freq = 1200 },
    { .type = toneext_t::GENERIC,      .len = 3000,  .freq = 1500 },
    { .type = toneext_t::SCAN_Y,       .len = 0,     .freq = 0    },
    { .type = toneext_t::SCAN_RY_BY,   .len = 0,     .freq = 0    }
  }
};

const SSTVEXTMode_t Robot72 {
  .visCode = RADIOLIB_SSTVEXT_ROBOT_72,
  .width = 320,
  .height = 240,
  .scanPixelLen = 4312,
  .numTones = 4,
  .tones = {
    { .type = toneext_t::GENERIC,      .len = 9000,  .freq = 1200 },
    { .type = toneext_t::GENERIC,      .len = 3000,  .freq = 1500 },
    { .type = toneext_t::SCAN_Y,       .len = 0,     .freq = 0    },
    { .type = toneext_t::SCAN_RY_BY,   .len = 0,     .freq = 0    }
  }
};

SSTVEXTClient::SSTVEXTClient(PhysicalLayer* phy) {
  phyLayer = phy;
  #if !RADIOLIB_EXCLUDE_AFSK
  audioClient = nullptr;
  #endif
}

#if !RADIOLIB_EXCLUDE_AFSK
SSTVEXTClient::SSTVEXTClient(AFSKClient* audio) {
  phyLayer = audio->phyLayer;
  audioClient = audio;
}
#endif

#if !RADIOLIB_EXCLUDE_AFSK
int16_t SSTVEXTClient::begin(const SSTVEXTMode_t& mode) {
  if(audioClient == nullptr) {
    // this initialization method can only be used in AFSK mode
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  return(begin(0, mode));
}
#endif

int16_t SSTVEXTClient::begin(float base, const SSTVEXTMode_t& mode) {
  // save mode
  txMode = mode;

  // calculate 24-bit frequency
  baseFreq = (base * 1000000.0) / phyLayer->getFreqStep();

  // configure for direct mode
  return(phyLayer->startDirect());
}

int16_t SSTVEXTClient::setCorrection(float correction) {
  // save correction factor
  correctionFactor = correction;

  // check if mode is initialized
  if(txMode.visCode == 0) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // apply correction factor to all timings
  txMode.scanPixelLen *= correction;
  for(uint8_t i = 0; i < txMode.numTones; i++) {
    txMode.tones[i].len *= correction;
  }
  return(RADIOLIB_ERR_NONE);
}

void SSTVEXTClient::idle() {
  phyLayer->transmitDirect();
  this->tone(RADIOLIB_SSTVEXT_TONE_LEADER);
}

void SSTVEXTClient::sendHeader() {
  // save first header flag for Scottie modes
  firstLine = true;
  phyLayer->transmitDirect();

  // preamble
  // this->tone(1900, 100);
  // this->tone(1500, 100);
  // this->tone(1900, 100);
  // this->tone(1500, 100);
  // this->tone(2300, 100);
  // this->tone(1500, 100);
  // this->tone(2300, 100);
  // this->tone(1500, 100);
  // this->tone(1900, 300);
  // this->tone(1200, 10);
  // this->tone(1900, 300);

  // send the first part of header (leader-break-leader)
  this->tone(RADIOLIB_SSTVEXT_TONE_LEADER, RADIOLIB_SSTVEXT_HEADER_LEADER_LENGTH);
  this->tone(RADIOLIB_SSTVEXT_TONE_BREAK, RADIOLIB_SSTVEXT_HEADER_BREAK_LENGTH);
  this->tone(RADIOLIB_SSTVEXT_TONE_LEADER, RADIOLIB_SSTVEXT_HEADER_LEADER_LENGTH);

  // VIS start bit
  this->tone(RADIOLIB_SSTVEXT_TONE_BREAK, RADIOLIB_SSTVEXT_HEADER_BIT_LENGTH);

  // VIS code
  uint8_t parityCount = 0;
  for(uint8_t mask = 0x01; mask < 0x80; mask <<= 1) {
    if(txMode.visCode & mask) {
      this->tone(RADIOLIB_SSTVEXT_TONE_VIS_1, RADIOLIB_SSTVEXT_HEADER_BIT_LENGTH);
      parityCount++;
    } else {
      this->tone(RADIOLIB_SSTVEXT_TONE_VIS_0, RADIOLIB_SSTVEXT_HEADER_BIT_LENGTH);
    }
  }

  // VIS parity
  if(parityCount % 2 == 0) {
    // even parity
    this->tone(RADIOLIB_SSTVEXT_TONE_VIS_0, RADIOLIB_SSTVEXT_HEADER_BIT_LENGTH);
  } else {
    // odd parity
    this->tone(RADIOLIB_SSTVEXT_TONE_VIS_1, RADIOLIB_SSTVEXT_HEADER_BIT_LENGTH);
  }

  // VIS stop bit
  this->tone(RADIOLIB_SSTVEXT_TONE_BREAK, RADIOLIB_SSTVEXT_HEADER_BIT_LENGTH);
}

/*
  * Order of Operations for Robot36:
  * 1. Figure out if the mode is Robot 36 or Robot 72
  * 2. If Robot36 then send Y Scan tone for calculated Hz value for 88000 ms
  * 3. If Robot36 and firstLine send an "even" separator 1500 Hz tone for 4500 ms 
  *    then 1900 Hz tone for 1500 ms then R-Y Scan tone for 0 Hz for 44000 ms
  * 4. If Robot36 and not firstLine and lastLine send an "even" separator 1500 Hz tone for 4500 ms 
  *    then 1900 Hz tone for 1500 ms then R-Y Scan tone for calculated Hz value for 44000 ms
  * 5. If Robot36 and not firstLine and not lastLine send an "odd" separator 2300 Hz tone for 4500 ms
  *    then 1900 Hz tone for 1500 ms then B-Y Scan tone for calculated Hz value for 44000 ms
*/

/*
  * Order of Operations for Robot72:
  * 1. Figure out if the mode is Robot 36 or Robot 72
  * 2. If Robot72 then send Y Scan tone for calculated Hz value for 138000 ms
  * 3. If Robot72 and firstLine send an "even" separator 1500 Hz tone for 4500 ms 
  *    then 1900 Hz tone for 1500 ms then R-Y Scan tone for 0 Hz for 69000 ms
  * 4. If Robot72 and not firstLine and lastLine send an "even" separator 1500 Hz tone for 4500 ms 
  *    then 1900 Hz tone for 1500 ms then R-Y Scan tone for calculated Hz value for 69000 ms
  * 5. If Robot72 and not firstLine and not lastLine send an "odd" separator 2300 Hz tone for 4500 ms
  *    then 1500 Hz tone for 1500 ms then B-Y Scan tone for calculated Hz value for 69000 ms
*/

void SSTVEXTClient::findTone() {
  if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_36) && (lastLine == true)) {
    // Even separator tone
    this->tone(RADIOLIB_SSTVEXT_TONE_ROBOT36_EVEN, (4500 * correctionFactor));
    // Porch tone
    this->tone(RADIOLIB_SSTVEXT_TONE_ROBOT36_PORCH, (1500 * correctionFactor));
  } else if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_36) && (lastLine == false)) {
    // Odd separator tone
    this->tone(RADIOLIB_SSTVEXT_TONE_ROBOT36_ODD, (4500 * correctionFactor));
    // Porch tone
    this->tone(RADIOLIB_SSTVEXT_TONE_ROBOT36_PORCH, (1500 * correctionFactor));
  }
}

// void SSTVEXTClient::makeTask() {

// }

void SSTVEXTClient::sendLine(const uint32_t* imgLine) {
  // send all tones in sequence
  for(uint8_t i = 0; i < txMode.numTones; i++) {
    if((txMode.tones[i].type == toneext_t::GENERIC) && (txMode.tones[i].len > 0)) {
      // sync/porch tones
      this->tone(txMode.tones[i].freq, txMode.tones[i].len);
    // } else if (txMode.tones[i].type == toneext_t::SCAN_RY_BY) {
    } else {
      if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_36) && (lastLine == true)) {
        // Even separator tone
        this->tone(1500, (4500 * correctionFactor));
        // Porch tone
        this->tone(1900, (1500 * correctionFactor));
      } else if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_36) && (lastLine == false)) {
        // Odd separator tone
        this->tone(2300, (4500 * correctionFactor));
        // Porch tone
        this->tone(1900, (1500 * correctionFactor));
      }
      if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_72) && (txMode.tones[i].type == toneext_t::SCAN_RY_BY) && (lastLine == true)) {
        // Even separator tone
        this->tone(1500, (4500 * correctionFactor));
        // Porch tone
        this->tone(1900, (1500 * correctionFactor));
        // BY Scan tone
        lastLine = false;
        txMode.tones[i].type = toneext_t::SCAN_RY;
      } else if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_72) && (txMode.tones[i].type == toneext_t::SCAN_RY_BY) && (lastLine == false)) {
        // Odd separator tone
        this->tone(2300, (4500 * correctionFactor));
        // Porch tone
        this->tone(1500, (1500 * correctionFactor));
        // RY Scan tone
        lastLine = true;
        txMode.tones[i].type = toneext_t::SCAN_BY;
      }
      // scan lines
      for(uint16_t j = 0; j < txMode.width; j++) {
        uint32_t color = imgLine[j];
        uint8_t r = (color &= 0x00FF0000) >> 16;
        uint8_t g = (color &= 0x0000FF00) >> 8;
        uint8_t b = color &= 0x000000FF;
        float v = 0.0;
        float vry = 0.0;
        float vby = 0.0;
        // TODO: I need to do the average of lines 1-2 for R-Y and then 2-3 for B-Y and then 3-4 for R-Y and so on.
        switch(txMode.tones[i].type) {
          case(toneext_t::SCAN_Y):
            v = 16.0 + (0.003906 * ((65.738 * r) + (129.057 * g) + (25.064 * b)));
            break;
          case(toneext_t::SCAN_RY):
            v = 128.0 + (0.003906 * ((112.439 * r) + (-94.154 * g) + (-18.285 * b)));
            break;
          case(toneext_t::SCAN_BY):
            v = 128.0 + (0.003906 * ((-37.945 * r) + (-74.494 * g) + (112.439 * b)));
            break;
          case(toneext_t::SCAN_RY_BY):
            vry = 128.0 + (0.003906 * ((112.439 * r) + (-94.154 * g) + (-18.285 * b)));
            vby = 128.0 + (0.003906 * ((-37.945 * r) + (-74.494 * g) + (112.439 * b)));
            break;
          case(toneext_t::GENERIC):
            break;
        }
        if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_36) && (txMode.tones[i].type == toneext_t::SCAN_Y)) {
          this->tone(RADIOLIB_SSTVEXT_TONE_BRIGHTNESS_MIN + ((float)v * 3.1372549), ((88000 / txMode.width) * correctionFactor));
        } else if ((txMode.tones[i].type == toneext_t::SCAN_RY_BY) && (lastLine == true)) {
          previousValueRY[j] = currentValueRY[j];
          currentValueRY[j] = (RADIOLIB_SSTVEXT_TONE_BRIGHTNESS_MIN + ((float)vry * 3.1372549));
          averageValueRY = ((previousValueRY[j] + currentValueRY[j]) / 2);
          currentValueBY[j] = (RADIOLIB_SSTVEXT_TONE_BRIGHTNESS_MIN + ((float)vby * 3.1372549));

          this->tone(averageValueRY, ((44000 / txMode.width) * correctionFactor));
        } else if ((txMode.tones[i].type == toneext_t::SCAN_RY_BY) && (lastLine == false)) {
          previousValueBY[j] = currentValueBY[j];
          currentValueBY[j] = (RADIOLIB_SSTVEXT_TONE_BRIGHTNESS_MIN + ((float)vby * 3.1372549));
          averageValueBY = ((previousValueBY[j] + currentValueBY[j]) / 2);
          currentValueRY[j] = (RADIOLIB_SSTVEXT_TONE_BRIGHTNESS_MIN + ((float)vry * 3.1372549));

          this->tone(averageValueBY, ((44000 / txMode.width) * correctionFactor));
        }
        if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_72) && (txMode.tones[i].type == toneext_t::SCAN_Y)) {
          this->tone(RADIOLIB_SSTVEXT_TONE_BRIGHTNESS_MIN + ((float)v * 3.1372549), ((138000 / txMode.width) * correctionFactor));
        } else if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_72) && ((txMode.tones[i].type == toneext_t::SCAN_RY) || (txMode.tones[i].type == toneext_t::SCAN_BY))) {
          this->tone(RADIOLIB_SSTVEXT_TONE_BRIGHTNESS_MIN + ((float)v * 3.1372549), ((69000 / txMode.width) * correctionFactor));
        }
        if (j == (txMode.width - 1)) {
          if ((txMode.tones[i].type == toneext_t::SCAN_RY) || (txMode.tones[i].type == toneext_t::SCAN_BY)) {
            txMode.tones[i].type = toneext_t::SCAN_RY_BY;
          }
          if (lastLine == true) {
            lastLine = false;
          } else if (lastLine == false) {
            lastLine = true;
          }
        }
      }
    }
  }
}

uint16_t SSTVEXTClient::getPictureHeight() const {
  return(txMode.height);
}

void SSTVEXTClient::tone(float freq, RadioLibTime_t len) {
  Module* mod = phyLayer->getMod();
  RadioLibTime_t start = mod->hal->micros();
  #if !RADIOLIB_EXCLUDE_AFSK
  if(audioClient != nullptr) {
    audioClient->tone(freq, false);
  } else {
    phyLayer->transmitDirect(baseFreq + (freq / phyLayer->getFreqStep()));
  }
  #else
  phyLayer->transmitDirect(baseFreq + (freq / phyLayer->getFreqStep()));
  #endif
  mod->waitForMicroseconds(start, len);
}

#endif