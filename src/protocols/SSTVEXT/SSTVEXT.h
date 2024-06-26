#if !defined(_RADIOLIB_SSTVEXT_H)
#define _RADIOLIB_SSTVEXT_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_SSTVEXT

#include "../PhysicalLayer/PhysicalLayer.h"
#include "../AFSK/AFSK.h"

// the following implementation is based on information from
// http://www.barberdsp.com/downloads/Dayton%20Paper.pdf

// VIS codes
#define RADIOLIB_SSTVEXT_ROBOT_36                              8
#define RADIOLIB_SSTVEXT_ROBOT_72                              12

// SSTVEXT tones in Hz
#define RADIOLIB_SSTVEXT_TONE_LEADER                           1900
#define RADIOLIB_SSTVEXT_TONE_BREAK                            1200
#define RADIOLIB_SSTVEXT_TONE_VIS_1                            1100
#define RADIOLIB_SSTVEXT_TONE_VIS_0                            1300
#define RADIOLIB_SSTVEXT_TONE_BRIGHTNESS_MIN                   1500
#define RADIOLIB_SSTVEXT_TONE_BRIGHTNESS_MAX                   2300

// calibration header timing in us
#define RADIOLIB_SSTVEXT_HEADER_LEADER_LENGTH                  300000
#define RADIOLIB_SSTVEXT_HEADER_BREAK_LENGTH                   10000
#define RADIOLIB_SSTVEXT_HEADER_BIT_LENGTH                     30000

/*!
  \struct toneext_t
  \brief Structure to save data about tone.
*/
struct toneext_t {

  /*!
    \brief Tone type: GENERIC for sync and porch tones, OTHER for storing temp tone, SCAN_Y, SCAN_BY and SCAN_RY for scan lines.
  */
  enum {
    GENERIC = 0,
    OTHER,
    SCAN_Y,
    SCAN_BY,
    SCAN_RY,
  } type;

  /*!
    \brief Length of tone in us, set to 0 for picture scan tones.
  */
  RadioLibTime_t len;

  /*!
    \brief Frequency of tone in Hz, set to 0 for picture scan tones.
  */
  uint16_t freq;
};

/*!
  \struct SSTVEXTMode_t
  \brief Structure to save data about supported SSTVEXT modes.
*/
struct SSTVEXTMode_t {

  /*!
    \brief Unique VIS code of the SSTVEXT mode.
  */
  uint8_t visCode;

  /*!
    \brief Picture width in pixels.
  */
  uint16_t width;

  /*!
    \brief Picture height in pixels.
  */
  uint16_t height;

  /*!
    \brief Pixel scan length in us.
  */
  uint16_t scanPixelLen;

  /*!
    \brief Number of tones in each transmission line. Picture scan data is considered single tone.
  */
  uint8_t numTones;

  /*!
    \brief Sequence of tones in each transmission line. This is used to create the correct encoding sequence.
  */
  toneext_t tones[8];
};

// all currently supported SSTVEXT modes
extern const SSTVEXTMode_t Robot36;
extern const SSTVEXTMode_t Robot72;

/*!
  \class SSTVEXTClient
  \brief Client for SSTVEXT transmissions.
*/
class SSTVEXTClient {
  public:
    /*!
      \brief Constructor for 2-FSK mode.
      \param phy Pointer to the wireless module providing PhysicalLayer communication.
    */
    explicit SSTVEXTClient(PhysicalLayer* phy);

    #if !RADIOLIB_EXCLUDE_AFSK
    /*!
      \brief Constructor for AFSK mode.
      \param audio Pointer to the AFSK instance providing audio.
    */
    explicit SSTVEXTClient(AFSKClient* audio);
    #endif

    // basic methods

    /*!
      \brief Initialization method for 2-FSK.
      \param base Base "0 Hz tone" RF frequency to be used in MHz.
      \param mode SSTVEXT mode to be used. Currently supported modes are Robot36 and Robot72.
      \returns \ref status_codes
    */
    int16_t begin(float base, const SSTVEXTMode_t& mode);

    #if !RADIOLIB_EXCLUDE_AFSK
    /*!
      \brief Initialization method for AFSK.
      \param mode SSTVEXT mode to be used. Currently supported modes are Robot36 and Robot 72.
      \returns \ref status_codes
    */
    int16_t begin(const SSTVEXTMode_t& mode);
    #endif

    /*!
      \brief Set correction coefficient for tone length.
      \param correction Timing correction factor, used to adjust the length of timing pulses.
      Less than 1.0 leads to shorter timing pulses, defaults to 1.0 (no correction).
      \returns \ref status_codes
    */
    int16_t setCorrection(float correction);

    /*!
      \brief Sends out tone at 1900 Hz.
    */
    void idle();

    /*!
      \brief Sends synchronization header for the SSTVEXT mode set in begin method.
    */
    void sendHeader();

    /*!
      \brief Sends single picture line in the currently configured SSTVEXT mode.
      \param imgLine Image line to send, in 24-bit RGB. It is up to the user to ensure that
      imgLine has enough pixels to send it in the current SSTVEXT mode.
    */
    void sendLine(const uint32_t* imgLine);

    /*!
      \brief Get picture height of the currently configured SSTVEXT mode.
      \returns Picture height of the currently configured SSTVEXT mode in pixels.
    */
    uint16_t getPictureHeight() const;

#if !RADIOLIB_GODMODE
  private:
#endif
    PhysicalLayer* phyLayer;
    #if !RADIOLIB_EXCLUDE_AFSK
    AFSKClient* audioClient;
    #endif

    uint32_t baseFreq = 0;
    SSTVEXTMode_t txMode = Robot36;
    bool firstLine = true;
    bool lastLine = true;
    float correctionFactor = 1.0;

    void tone(float freq, RadioLibTime_t len = 0);
};

#endif

#endif
