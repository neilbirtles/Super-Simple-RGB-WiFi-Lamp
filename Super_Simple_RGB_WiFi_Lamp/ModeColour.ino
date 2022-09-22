class ModeColour : public ModeBase
{
private:
    // Config
    int colourRed   = 128;
    int colourGreen = 128;
    int colourBlue  = 128;
    int colourWhite = 0;
    int colorBrightness = 100;
public:
    ModeColour() {}
    virtual void initialize() {}

    virtual void render() {
      int brightness = colorBrightness;
      brightness = constrain(brightness, 0, 255);
      
      if (colourWhite > 0){
        //have a white request so fill white 
        fill_solid(ledString, NUM_LEDS, CRGBW(0, 0, 0, 255));               
        
      }else{
        //otherwise fill with selected colours
        fill_solid(ledString, NUM_LEDS, CRGB(colourRed, colourGreen, colourBlue));
      }

      FastLED.setBrightness(brightness);
    }

    virtual void applyConfig(JsonVariant& settings) {
        settings["Red"] = colourRed = settings["Red"] | colourRed;
        settings["Green"] = colourGreen = settings["Green"] | colourGreen;
        settings["Blue"] = colourBlue = settings["Blue"] | colourBlue;
        settings["White"] = colourWhite = settings["White"] | colourWhite;
        settings["Brightness"] = colorBrightness = settings["Brightness"] | colorBrightness;
    }
};
