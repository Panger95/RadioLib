#include "SSTVEXT.h"
#if !RADIOLIB_EXCLUDE_SSTVEXT

const SSTVEXTMode_t Robot36 {
  .visCode = RADIOLIB_SSTVEXT_ROBOT_36,
  .width = 320,
  .height = 240,
  .scanPixelLen = 2750,
  .numTones = 6,
  .tones = {
    { .type = toneext_t::GENERIC,    .len = 9000,   .freq = 1200 },
    { .type = toneext_t::GENERIC,    .len = 3000,   .freq = 1500 },
    { .type = toneext_t::SCAN_Y,     .len = 0,      .freq = 0    },
    { .type = toneext_t::GENERIC,    .len = 4500,   .freq = 1500 },
    { .type = toneext_t::GENERIC,    .len = 1500,   .freq = 1900 },
    { .type = toneext_t::OTHER,      .len = 0,      .freq = 0    }
  }
};

const SSTVEXTMode_t Robot72 {
  .visCode = RADIOLIB_SSTVEXT_ROBOT_72,
  .width = 320,
  .height = 240,
  .scanPixelLen = 4312,
  .numTones = 4,
  .tones = {
    { .type = toneext_t::GENERIC,    .len = 9000,   .freq = 1200 },
    { .type = toneext_t::GENERIC,    .len = 3000,   .freq = 1500 },
    { .type = toneext_t::SCAN_Y,     .len = 0,      .freq = 0    },
    { .type = toneext_t::OTHER,      .len = 0,      .freq = 0    }
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
    // this EXTinitialization method can only be used in AFSK mode
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

void SSTVEXTClient::sendLine(const uint32_t* imgLine) {
  // send all tones in sequence
  for(uint8_t i = 0; i < txMode.numTones; i++) {
    if((txMode.tones[i].type == toneext_t::GENERIC) && (txMode.tones[i].len > 0)) {
      // sync/porch tones
      this->tone(txMode.tones[i].freq, txMode.tones[i].len);
    } else {
      if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_36) && (txMode.tones[i].type == toneext_t::OTHER) && (lastLine == true)) {
        // Even separator tone
        this->tone(1500, 4500);
        // Porch tone
        this->tone(1900, 1500);
        // RY Scan tone
        lastLine = false;
        txMode.tones[i].type = toneext_t::SCAN_RY;
      }
      else if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_36) && (txMode.tones[i].type == toneext_t::OTHER) && (lastLine == false)) {
        // Odd separator tone
        this->tone(2300, 4500);
        // Porch tone
        this->tone(1900, 1500);
        // BY Scan tone
        lastLine = true;
        txMode.tones[i].type = toneext_t::SCAN_BY;
      }
      if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_72) && (txMode.tones[i].type == toneext_t::OTHER) && (lastLine == true)) {
        // Even separator tone
        this->tone(1500, 4500);
        // Porch tone
        this->tone(1900, 1500);
        // BY Scan tone
        lastLine = false;
        txMode.tones[i].type = toneext_t::SCAN_RY;
      } else if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_72) && (txMode.tones[i].type == toneext_t::OTHER) && (lastLine == false)) {
        // Odd separator tone
        this->tone(2300, 4500);
        // Porch tone
        this->tone(1500, 1500);
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
        float v;
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
          case(toneext_t::GENERIC):
            break;
          case(toneext_t::OTHER):
            break;
        }
        if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_36) && (txMode.tones[i].type == toneext_t::SCAN_Y)) {
          this->tone(RADIOLIB_SSTVEXT_TONE_BRIGHTNESS_MIN + ((float)v * 3.1372549), 88000);
        }
        else if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_36) && ((txMode.tones[i].type == toneext_t::SCAN_RY) || (txMode.tones[i].type == toneext_t::SCAN_BY))) {
          this->tone(RADIOLIB_SSTVEXT_TONE_BRIGHTNESS_MIN + ((float)v * 3.1372549), 44000);
        }
        if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_72) && (txMode.tones[i].type == toneext_t::SCAN_Y)) {
          this->tone(RADIOLIB_SSTVEXT_TONE_BRIGHTNESS_MIN + ((float)v * 3.1372549), 138000);
        }
        else if ((txMode.visCode == RADIOLIB_SSTVEXT_ROBOT_72) && ((txMode.tones[i].type == toneext_t::SCAN_RY) || (txMode.tones[i].type == toneext_t::SCAN_BY))) {
          this->tone(RADIOLIB_SSTVEXT_TONE_BRIGHTNESS_MIN + ((float)v * 3.1372549), 69000);
        }
        if ((j == (txMode.width - 1)) && ((txMode.tones[i].type == toneext_t::SCAN_RY) || (txMode.tones[i].type == toneext_t::SCAN_BY))) {
          txMode.tones[i].type = toneext_t::OTHER;
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
