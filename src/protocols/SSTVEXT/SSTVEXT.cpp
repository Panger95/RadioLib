#include "SSTVEXT.h"
#if !RADIOLIB_EXCLUDE_SSTVEXT

const SSTVEXTMode_t Robot36 {
  .visCode = RADIOLIB_SSTVEXT_ROBOT_36,
  .width = 320,
  .height = 240,
  .scanPixelLen = 88000,
  .numTones = 12,
  .tones = {
    { .type = tone_t::GENERIC,    .len = 9000,    .freq = 1200 },
    { .type = tone_t::GENERIC,    .len = 3000,    .freq = 1500 },
    { .type = tone_t::SCAN_Y,     .len = 88000,   .freq = 0    },
    { .type = tone_t::GENERIC,    .len = 4500,    .freq = 1500 },
    { .type = tone_t::GENERIC,    .len = 1500,    .freq = 1900 },
    { .type = tone_t::SCAN_RY,    .len = 44000,   .freq = 0    },
    { .type = tone_t::GENERIC,    .len = 9000,    .freq = 1200 },
    { .type = tone_t::GENERIC,    .len = 3000,    .freq = 1500 },
    { .type = tone_t::SCAN_Y,     .len = 88000,   .freq = 0    },
    { .type = tone_t::GENERIC,    .len = 4500,    .freq = 2300 },
    { .type = tone_t::GENERIC,    .len = 1500,    .freq = 1900 },
    { .type = tone_t::SCAN_BY,    .len = 44000,   .freq = 0    },
  }
};

const SSTVEXTMode_t Robot72 {
  .visCode = RADIOLIB_SSTVEXT_ROBOT_72,
  .width = 320,
  .height = 240,
  .scanPixelLen = 138000,
  .numTones = 9,
  .tones = {
    { .type = tone_t::GENERIC,    .len = 9000,    .freq = 1200 },
    { .type = tone_t::GENERIC,    .len = 3000,    .freq = 1500 },
    { .type = tone_t::SCAN_Y,     .len = 138000,  .freq = 0    },
    { .type = tone_t::GENERIC,    .len = 4500,    .freq = 1500 },
    { .type = tone_t::GENERIC,    .len = 1500,    .freq = 1900 },
    { .type = tone_t::SCAN_RY,    .len = 69000,   .freq = 0    },
    { .type = tone_t::GENERIC,    .len = 4500,    .freq = 2300 },
    { .type = tone_t::GENERIC,    .len = 1500,    .freq = 1500 },
    { .type = tone_t::SCAN_BY,    .len = 69000,   .freq = 0    },
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
    if((txMode.tones[i].type == tone_t::GENERIC) && (txMode.tones[i].len > 0)) {
      // sync/porch tones
      this->tone(txMode.tones[i].freq, txMode.tones[i].len);
    } else {
      // scan lines
      for(uint16_t j = 0; j < txMode.width; j++) {
        uint32_t color = imgLine[j];
        uint32_t r = (color & 0x00FF0000) >> 16;
        uint32_t g = (color & 0x0000FF00) >> 8;
        uint32_t b = color & 0x000000FF;
        uint32_t v;
        switch(txMode.tones[i].type) {
          case(tone_t::SCAN_Y):
            v = 16 + (0.003906 * ((65.738 * r) + (129.057 * g) + (25.064 * b)));
            break;
          case(tone_t::SCAN_RY):
            v = 128.0 + (0.003906 * ((112.439 * r) + (-94.154 * g) + (-18.285 * b)));
            break;
          case(tone_t::SCAN_BY):
            v = 128.0 + (0.003906 * ((-37.945 * r) + (-74.494 * g) + (112.439 * b)));
            break;
          case(tone_t::GENERIC):
            break;
        }
        this->tone(RADIOLIB_SSTVEXT_TONE_BRIGHTNESS_MIN + ((float)v * 3.1372549), txMode.tones[i].len);
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
