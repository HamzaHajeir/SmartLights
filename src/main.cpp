#include <Arduino.h>
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#include <FastLED.h> // To compile with H4Plugins, Change all occurances of OutputPin  to FastOutputPin, then compile.
#include <H4Plugins.h>
#include <pmbtools.h>
#include <vector>
#define NUM_LEDS 380
#define BRIGHTNESS 64
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
CRGBPalette16 currentPalette;
TBlendType currentBlending;
using CurrentTheme = std::pair<CRGBPalette16, TBlendType>;
std::vector<CurrentTheme> cThemeVec;
int lastIndex = 0;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;
extern CRGBPalette16 myRedWhiteBluePalette;

#define LED_PIN D2
constexpr int buttonPin = D5;

void visualLEDsSetup();
void visualLEDsLoop();
void updatePalette();
// void SelectPalette();
CRGBPalette16 SetupBlackStriped();
CRGBPalette16 SetupBlackAndWhiteStripedPalette();
CRGBPalette16 SetupPurpleAndGreenPalette();
CRGBPalette16 SetupTotallyRandomPalette();
void ChangePalettePeriodically();
void FillLEDsFromPaletteColors(uint8_t colorIndex);
void setupNext();
// constexpr uint8_t NO_OF_PALETTES = 10;

H4_USE_PLUGINS(115200, 20, false)
void onGlobalsChange(const std::string &svc, H4PE_TYPE t, const std::string &value);
void onMQTTConnect();

H4P_PinMachine h4pm;
H4P_WiFi h4wifi("SSID","PASSWORD");
H4P_AsyncMQTT h4mqtt("http://192.168.1.4:1883");
h4pTactless myButton(buttonPin, INPUT_PULLUP, ACTIVE_LOW, /*Debounce time (ms)=*/ 20);

void h4pGlobalEventHandler(const std::string &svc, H4PE_TYPE t, const std::string &msg)
{
    switch (t)
    {
        // provides default actions for H4PE_SYSINFO, H4PE_SYSWARN, H4PE_SYSFATAL
        H4P_DEFAULT_SYSTEM_HANDLER
    default:
        break;
    }
}


H4P_EventListener gpio(H4PE_GPIO, [](const std::string &pin, H4PE_TYPE t, const std::string &msg)
                       {
                           int p = atoi(pin.c_str());
                           int v = atoi(msg.c_str());
                           switch (p)
                           {
                           case buttonPin:
                           {
                               setupNext();
                           }
                           break;
                           default:
                               break;
                           }
                       });

H4P_EventListener gvChanges(H4PE_GVCHANGE, onGlobalsChange);

void h4setup()
{
    visualLEDsSetup();
    if (!h4p.gvExists("palette"))
    {
        h4p.gvSetInt("palette", 0, true);
    }
}

void h4UserLoop()
{
    visualLEDsLoop();
}

void validateRegisty()
{
    if (!h4p.gvExists("palette"))
        h4p.gvSetInt("palette", 1, true);

    int paletteNo = h4p.gvGetInt("palette");
    if (paletteNo < 0 || paletteNo >= cThemeVec.size())
    {
        h4p.gvSetInt("palette", 1, true);
    }
}
void visualLEDsSetup()
{
    cThemeVec.push_back(std::make_pair<CRGBPalette16, TBlendType>(SetupBlackStriped(), LINEARBLEND));
    cThemeVec.push_back(std::make_pair<CRGBPalette16, TBlendType>(RainbowColors_p, LINEARBLEND));
    cThemeVec.push_back(std::make_pair<CRGBPalette16, TBlendType>(SetupBlackAndWhiteStripedPalette(), LINEARBLEND));
    cThemeVec.push_back(std::make_pair<CRGBPalette16, TBlendType>(CloudColors_p, LINEARBLEND));
    cThemeVec.push_back(std::make_pair<CRGBPalette16, TBlendType>(HeatColors_p, LINEARBLEND));
    cThemeVec.push_back(std::make_pair<CRGBPalette16, TBlendType>(LavaColors_p, LINEARBLEND));
    cThemeVec.push_back(std::make_pair<CRGBPalette16, TBlendType>(ForestColors_p, LINEARBLEND));
    cThemeVec.push_back(std::make_pair<CRGBPalette16, TBlendType>(PartyColors_p, LINEARBLEND));
    cThemeVec.push_back(std::make_pair<CRGBPalette16, TBlendType>(myRedWhiteBluePalette_p, LINEARBLEND));
    cThemeVec.push_back(std::make_pair<CRGBPalette16, TBlendType>(SetupPurpleAndGreenPalette(), LINEARBLEND));
    cThemeVec.push_back(std::make_pair<CRGBPalette16, TBlendType>(SetupTotallyRandomPalette(), LINEARBLEND));

    FastLED.addLeds<WS2812B, 2, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);

    h4.every(50, //call below void function every 15 ms
             visualLEDsLoop);

    validateRegisty();
    h4mqtt.publishDevice("debug", "setup");
    updatePalette();
    h4mqtt.publishDevice("debug", "pallete selected");
}

void visualLEDsLoop()
{
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* motion speed */

    uint8_t colorIndex = startIndex;
    uint8_t brightness = 255;

    for (int i = 0; i < NUM_LEDS; i++)
    {
        leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }

    FastLED.show();
    // SelectPalette();
    static uint32_t count = 0;
    count++;
    if (count % 100 == 0)
        h4mqtt.publishDevice("debug", "updated 100 times");
}

void updatePalette()
{
    lastIndex = h4p.gvGetInt("palette");
    CurrentTheme cTheme = cThemeVec.at(lastIndex);
    currentPalette = cTheme.first;
    currentBlending = cTheme.second;
}


void dumpvs(const std::vector<std::string> &vs)
{
    for (auto const &v : vs)
        Serial.printf("%s\n", v.data());
}

uint32_t updateColorPalette(std::vector<std::string> vs)
{
    H4_DEBUG(dumpvs(vs));
    if (stringIsNumeric(H4PAYLOAD))
    {
        if (H4PAYLOAD_INT >= 0 && H4PAYLOAD_INT < cThemeVec.size() /* NO_OF_PALETTES */)
        {
            h4p.gvSetInt("palette", H4PAYLOAD_INT, true);
            return H4_CMD_OK;
        }
        else
        {
            return H4_CMD_OUT_OF_BOUNDS;
        }
    }
    else
    {
        return H4_CMD_NOT_NUMERIC;
    }
}

void onGlobalsChange(const std::string &name, H4PE_TYPE t, const std::string &value)
{
    if (name == "palette")
    {
        if (stringIsNumeric(value))
        {
            int val = atoi(CSTR(value));
            if (val >= 0 && val < cThemeVec.size())
            {
                updatePalette();
                return;
            }
        }
        // Return to last one.
        h4p.gvSetInt("palette", lastIndex, true);
    }
}


void setupNext()
{
    int v = h4p.gvGetInt("palette") + 1;
    v = (v == cThemeVec.size()) ? 0 : v;

    h4p.gvSetInt("palette", v, true);
}

// This function fills the palette with totally random colors.
CRGBPalette16 SetupTotallyRandomPalette()
{
    CRGBPalette16 palette;
    for (int i = 0; i < 16; i++)
    {
        palette[i] = CHSV(random8(), 255, random8());
    }
    return palette;
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
CRGBPalette16 SetupBlackAndWhiteStripedPalette()
{
    CRGBPalette16 palette;
    // 'black out' all 16 palette entries...
    fill_solid(palette, 16, CRGB::Black);
    // and set every fourth one to white.
    palette[0] = CRGB::White;
    palette[4] = CRGB::White;
    palette[8] = CRGB::White;
    palette[12] = CRGB::White;
    return palette;
}

CRGBPalette16 SetupBlackStriped()
{
    CRGBPalette16 palette;
    // 'black out' all 16 palette entries...
    fill_solid(palette, 16, CRGB::Black);
    return palette;
}


// This function sets up a palette of purple and green stripes.
CRGBPalette16 SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV(HUE_PURPLE, 255, 255);
    CRGB green = CHSV(HUE_GREEN, 255, 255);
    CRGB black = CRGB::Black;

    return CRGBPalette16(
        green, green, black, black,
        purple, purple, black, black,
        green, green, black, black,
        purple, purple, black, black);
}


// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
    {
        CRGB::Red,
        CRGB::Gray, // 'white' is too bright compared to red and blue
        CRGB::Blue,
        CRGB::Black,

        CRGB::Red,
        CRGB::Gray,
        CRGB::Blue,
        CRGB::Black,

        CRGB::Red,
        CRGB::Red,
        CRGB::Gray,
        CRGB::Gray,
        CRGB::Blue,
        CRGB::Blue,
        CRGB::Black,
        CRGB::Black};

/* void SelectPalette()
{
    uint8_t secondHand = (millis() / 1000) % 75;
    static uint8_t lastSecond = 99;

    if (lastSecond != secondHand)
    {
        lastSecond = secondHand;
        if (secondHand == 0){LEDSTRIP_DEBUG(DEBUG_F("sh=%d Playing RainbowColors_p\n", secondHand));currentPalette = RainbowColors_p;currentBlending = LINEARBLEND;}
        if (secondHand == 10){LEDSTRIP_DEBUG(DEBUG_F("sh=%d Playing RainbowStripeColors_p\n", secondHand));currentPalette = RainbowStripeColors_p;currentBlending = NOBLEND;}
        if (secondHand == 15){LEDSTRIP_DEBUG(DEBUG_F("sh=%d Playing RainbowStripeColors_p\n", secondHand));currentPalette = RainbowStripeColors_p;currentBlending = LINEARBLEND;}
        if (secondHand == 20){LEDSTRIP_DEBUG(DEBUG_F("sh=%d Playing SetupPurpleAndGreenPalette\n", secondHand));SetupPurpleAndGreenPalette();currentBlending = LINEARBLEND;}
        if (secondHand == 25){LEDSTRIP_DEBUG(DEBUG_F("sh=%d Playing SetupTotallyRandomPalette\n", secondHand));SetupTotallyRandomPalette();currentBlending = LINEARBLEND;}
        if (secondHand == 30){LEDSTRIP_DEBUG(DEBUG_F("sh=%d Playing SetupBlackAndWhiteStripedPalette\n", secondHand));SetupBlackAndWhiteStripedPalette();currentBlending = NOBLEND;}
        if (secondHand == 35){LEDSTRIP_DEBUG(DEBUG_F("sh=%d Playing SetupBlackAndWhiteStripedPalette\n", secondHand));SetupBlackAndWhiteStripedPalette();currentBlending = LINEARBLEND;}
        if (secondHand == 40){LEDSTRIP_DEBUG(DEBUG_F("sh=%d Playing CloudColors_p\n", secondHand));currentPalette = CloudColors_p;currentBlending = LINEARBLEND;}
        if (secondHand == 45){LEDSTRIP_DEBUG(DEBUG_F("sh=%d Playing PartyColors_p\n", secondHand));currentPalette = PartyColors_p;currentBlending = LINEARBLEND;}
        if (secondHand == 50){LEDSTRIP_DEBUG(DEBUG_F("sh=%d Playing myRedWhiteBluePalette_p\n", secondHand));currentPalette = myRedWhiteBluePalette_p;currentBlending = NOBLEND;}
        if (secondHand == 55){LEDSTRIP_DEBUG(DEBUG_F("sh=%d Playing myRedWhiteBluePalette_p\n", secondHand));currentPalette = myRedWhiteBluePalette_p;currentBlending = LINEARBLEND;}
        if (secondHand == 60){LEDSTRIP_DEBUG(DEBUG_F("sh=%d Playing HeatColors_p\n", secondHand));currentPalette = HeatColors_p;currentBlending = LINEARBLEND;}
        if (secondHand == 65){LEDSTRIP_DEBUG(DEBUG_F("sh=%d Playing ForestColors_p\n", secondHand));currentPalette = ForestColors_p;currentBlending = LINEARBLEND;}
        if (secondHand == 70){LEDSTRIP_DEBUG(DEBUG_F("sh=%d Playing LavaColors_p\n", secondHand));currentPalette = LavaColors_p;currentBlending = LINEARBLEND;}
        h4mqtt.publishDevice("debug","pallete selected");
    }
} */