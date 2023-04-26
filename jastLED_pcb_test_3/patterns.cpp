#include <FastLED.h>
#include <Arduino.h>
#include "arduinoFFT.h"
#include "patterns.h"
#include "nanolux_types.h"
#include "nanolux_util.h"
#include "palettes.h"
#include "audio_analysis.h"

extern CRGB leds[NUM_LEDS];        // Buffer (front)
extern CRGB hist[NUM_LEDS];        // Buffer (back)
extern unsigned int sampling_period_us;
extern unsigned long microseconds;
extern double vReal[SAMPLES];      // Sampling buffers
extern double vImag[SAMPLES];
extern double vRealHist[SAMPLES];  // for delta freq
extern double delt[SAMPLES];
extern int frame;                 // for spring mass
extern double amplitude;          //for spring mass 2
extern arduinoFFT FFT;
extern bool button_pressed;
extern SimplePatternList gPatterns;
extern int NUM_PATTERNS;
extern SimplePatternList gPatterns_layer;
extern uint8_t gCurrentPatternNumber;     // Index number of which pattern is current
extern uint8_t gHue;                      // rotating base color
extern double peak;                       //  peak frequency
extern uint8_t fHue;                      // hue value based on peak frequency
extern double volume;                     //  NOOOOTEEEE:  static??
extern uint8_t vbrightness;
extern double maxDelt;                    // Frequency with the biggest change in amp.
extern int frame;
extern int beats;
extern int tempHue;
extern int vol_pos;
extern int pix_pos;
extern bool vol_show;
extern uint8_t genre_smoothing[10];
extern int genre_pose;
extern int advanced_size;
extern double max1[20];
extern double max2[20];
extern double max3[20];
extern double max4[20];
extern double max5[20];
extern int maxIter;
CRGBPalette16 gPal = GMT_hot_gp; //store all palettes in array
CRGBPalette16 gPal2 = nrwc_gp; //store all palettes in array
bool gReverseDirection = false;
extern double velocity;
extern double acceleration;
extern double smoothing_value[10];
extern int location;
extern double velocities[5];
extern double accelerations[5];
extern int locations[5];
extern double vRealSums[5];

void nextPattern() {
  // add one to the current pattern number, and wrap around at the end
  // gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % NUM_PATTERNS;
  // gCurrentPatternNumber++;
}

void layer_patterns(){
  // Loop through number of patterns being layered
  for(int i = 0; i < NUM_PATTERNS; i++){
    // Get current pattern led values
    gPatterns_layer[i]();
  }
}

void setColorHSV(CRGB* leds, byte h, byte s, byte v) {
  // create a new HSV color
  CHSV color = CHSV(h, s, v);
  // use FastLED to set the color of all LEDs in the strip to the same color
  fill_solid(leds, NUM_LEDS, color);
}


void freq_hue_vol_brightness(){
  #ifdef DEBUG
    Serial.print("\t pattern: freq_hue_vol_brightness\t fHue: ");
    Serial.print(fHue);
    Serial.print("\t vbrightness: ");
    Serial.println(vbrightness);
  #endif
  CHSV color = CHSV(fHue, 255, vbrightness);
  fill_solid(leds, NUM_LEDS, color);
}


void freq_confetti_vol_brightness(){
  // colored speckles based on frequency that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( fHue + random8(10), 255, vbrightness);
  leds[pos] += CHSV( fHue + random8(10), 255, vbrightness);
}


void volume_level_middle_bar_freq_hue(){
  FastLED.clear();

  int n = remap(volume, MIN_VOLUME, MAX_VOLUME, 0, NUM_LEDS/2);
  int mid_point = (int) NUM_LEDS/2;
  
  for(int led = 0; led < n; led++) {
    leds[mid_point + led].setHue( fHue );
    leds[mid_point - led].setHue( fHue );
  }
}


void freq_hue_trail(){
  leds[0] = CHSV( fHue, 255, vbrightness);
  leds[1] = CHSV( fHue, 255, vbrightness);
  CRGB temp;
  
  for(int i = NUM_LEDS-1; i > 1; i-=2) {
      leds[i] = leds[i-2];
      leds[i-1] = leds[i-2];
  }
}


void blank(){
  FastLED.clear();
}


void spring_mass_1 (){
  int middle_mass_displacement = 0;
  int middle_position = NUM_LEDS / 2;
  int mass = 5;
  int mass_radius = 12;
  int spring_constant = 5;
  double period_avg = 0;
  double friction = 1;

  if (amplitude < middle_position) {
    amplitude += vbrightness / 7;
  } 

  middle_mass_displacement = amplitude*cos(sqrt(spring_constant/mass)*(frame)/3);
  frame++;
  
  if (amplitude > friction) {
    amplitude = amplitude - friction; 
  } else {
    amplitude = 0;
  }

  int left_end = middle_position + middle_mass_displacement - mass_radius;
  int right_end = middle_position + middle_mass_displacement + mass_radius;
  
  for (int i = 0; i < NUM_LEDS-1; i++)
  {
    if ((i > left_end) && (i < right_end))
    {
      int springbrightness = (90/mass_radius) * abs(mass_radius - abs(i - (middle_mass_displacement+middle_position)));
      leds[i] = CHSV (fHue, 255-vbrightness, springbrightness);
    } else {
      leds[i] = CHSV (0, 0, 0);
    }
  }
}

void spring_mass_2 () {
  int middle_position = NUM_LEDS / 2;
  int mass = 5;
  int mass_radius = 15;
  int friction = 100;
  double spring_constant = 0;

  for (int i = 10; i > 0; i--) {
    spring_constant += smoothing_value[i];
    smoothing_value[i] = smoothing_value[i-1];
  }
  smoothing_value[0] = fHue;
  spring_constant += fHue;
  spring_constant = spring_constant / 2550;
  
  acceleration = -1*location * spring_constant/mass;
  if (velocity > 0)
  {
    velocity += acceleration + (vbrightness/80);
  } else {
    velocity += acceleration - (vbrightness/80);
  }
  location += velocity;
  if (location < -1*NUM_LEDS/2)
  {
    location = -1*NUM_LEDS/2;
  } else if (location > NUM_LEDS/2) {
    location = NUM_LEDS/2;
  }

  int left_end = middle_position + location - mass_radius;
  int right_end = middle_position + location + mass_radius;
  
  for (int i = 0; i < NUM_LEDS-1; i++)
  {
    if ((i > left_end) && (i < right_end))
    {        
      int springbrightness = 90 - (90/mass_radius * abs(i - (location+middle_position)));
      leds[i] = CHSV (spring_constant * 255, 255-vbrightness, springbrightness);
    } else {
      leds[i] = CHSV (0, 0, 0);
    }
  }
}

void spring_mass_3() {
  int middle_position = NUM_LEDS / 2;
  int mass = 5;
  int mass_radiuses[5] = {6,5,4,3,2};
  int friction = 100;
  double spring_constants[5] = {0.05, 0.10, 0.15, 0.20, 0.25};
  double tempsum = 0;
  
  for (int k = 0; k < 5; k++){      
    for (int i = (3+(k*SAMPLES/5)); i < ((k+1)*SAMPLES/5)-3; i++) {
      tempsum +=  vReal[i];
    }
    vRealSums[k] = tempsum/(SAMPLES/5);
    vRealSums[k] = remap(vRealSums[k], MIN_VOLUME, MAX_VOLUME, 0, 5);
  }
  
  for (int j = 0; j < 5; j++) {
    accelerations[j] = -1*locations[j] * spring_constants[j]/mass;
    if (velocity > 0)
    {
      velocities[j] += accelerations[j] + (vRealSums[j]);
    } else {
      velocities[j] += accelerations[j] - (vRealSums[j]);
    }
    locations[j] += velocities[j];
    if (locations[j] < -1*NUM_LEDS/2)
    {
      locations[j] = -1*NUM_LEDS/2;
    } else if (locations[j] > NUM_LEDS/2) {
      locations[j] = NUM_LEDS/2;
    }

    int left_end = middle_position + locations[j] - mass_radiuses[j];
    int right_end = middle_position + locations[j] + mass_radiuses[j];
  
    for (int i = 0; i < NUM_LEDS-1; i++)
    {
      if ((i > left_end) && (i < right_end))
      {        
        //int springbrightness = 90 - (90/mass_radius * abs(i - (locations[j]+middle_position)));
        leds[i] = CHSV (spring_constants[j] * 255 * 4, 255-vbrightness, 80);
      } else {
        leds[i] -= CHSV (0, 0, 10);
      }
    }
  } 
}

void classical() {
  double* temp_formants = density_formant();
  int bpm = map(vbrightness, 0, MAX_BRIGHTNESS, 1, 30);
  uint16_t sin = beatsin16(bpm, 0, temp_formants[1]);

  fadeToBlackBy(leds, NUM_LEDS, 50);

  CRGBPalette16 myPal = hue_gp;
  fill_palette(leds, NUM_LEDS, sin, 255/NUM_LEDS, myPal, 50, LINEARBLEND);

  #ifdef DEBUG
      Serial.print("\t Classical: ");
  #endif

  delete[] temp_formants;
}

void pix_freq() {
  fadeToBlackBy(leds, NUM_LEDS, 50);
  if (volume > 200) {
    pix_pos = map(peak, MIN_FREQUENCY, MAX_FREQUENCY, 0, NUM_LEDS-1);
    tempHue = fHue;
  }
  else {
    pix_pos--;
    tempHue--;
    vol_pos--;
  }
  if (vol_show) {
    if (volume > 100) {
      vol_pos = map(volume, MIN_VOLUME, MAX_VOLUME, 0, NUM_LEDS-1);
      tempHue = fHue;
    } else {
      vol_pos--;
    }

    leds[vol_pos] = vol_pos < NUM_LEDS ? CRGB(255, 255, 255):CRGB(0, 0, 0);
  }
  leds[pix_pos] = pix_pos < NUM_LEDS ? CHSV(tempHue, 255, 255):CRGB(0, 0, 0);
}

void send_wave() {
  double change_by = vbrightness;
  int one_sine = map(change_by, 0, MAX_BRIGHTNESS, 25, 35);
  CRGB color = CRGB(0, one_sine/2, 50);
  fill_solid(leds, NUM_LEDS, color);
  uint8_t sinBeat = beatsin8(30, 0, NUM_LEDS-1, 0, 0);
  leds[sinBeat] = CRGB(10, 10, 0);
  fadeToBlackBy(leds, NUM_LEDS, 1);
  uint8_t sinBeat1 = beatsin8(one_sine, 0, NUM_LEDS-1, 0, 170);
  leds[sinBeat1] = CRGB(255, 0, 0);
  fadeToBlackBy(leds, NUM_LEDS, 1);
  uint8_t sinBeat2 = beatsin8(one_sine, 0, NUM_LEDS-1, 0, 255);
  leds[sinBeat2] = CRGB(255,255,255);
  fadeToBlackBy(leds, NUM_LEDS, 1);

  for (int i = 0; i < 20; i++) {
    blur1d(leds, NUM_LEDS, 50);
  }
  fadeToBlackBy(leds, NUM_LEDS, 50);

  #ifdef DEBUG
    Serial.print("\t sinBeat: ");
    Serial.print(sinBeat);
    Serial.print("\t sinBeat1: ");
    Serial.print(sinBeat1);
    Serial.print("\t sinBeat2: ");
    Serial.print(sinBeat2);
  #endif
}

void math() {
  int len = (sizeof(vReal)/sizeof(vReal[0])) / 10;
  double smol_arr[len];
  memcpy(smol_arr, vReal + (8*len), len-1);
  double sums = 0;
  for (int i = 0; i < len; i++) {
    sums += smol_arr[i];
  }
  int vol = sums/len;
  uint8_t brit = remap(vol, MIN_VOLUME, MAX_VOLUME, 0, MAX_BRIGHTNESS); 
  double red = 0.;
  double green = 0.;
  double blue = 0.;

  for (int i = 5; i < SAMPLES - 3; i++) {
    if (i < (SAMPLES-8)/3) {
      blue += vReal[i];
    }
    else if (i >= (SAMPLES-8)/3 && i < (2*(SAMPLES-8)/3)) {
      green += vReal[i];
    }
    else {
      red += vReal[i];
    }
  }

  red /= 300;
  blue /= 300;
  green /= 300;

  uint8_t hue = remap(log ( red + green + 2.5*blue ) / log ( 2 ), log ( MIN_FREQUENCY ) / log ( 2 ), log ( MAX_FREQUENCY ) / log ( 2 ), 10, 240);
  genre_smoothing[genre_pose] = hue;
  genre_pose++;
  if (genre_pose == 10) {
    genre_pose = 0;
  }

  
  int nue = 0;
  for (int i = 0; i < 10; i++) {
    nue += genre_smoothing[i];
  }
  hue = nue/8;

  frame++;

  CHSV color = CHSV(255-hue, 255, 4*brit);

  int mid_point = (int) NUM_LEDS/2;
  fill_solid(leds, NUM_LEDS, color);
  send_wave();

  red = map(red, 0, 1500, 0, (NUM_LEDS/2)-1);
  
  for(int led = 0; led < red; led++) {
    leds[mid_point + led] = CHSV(2*hue, 255, 3*brit);
    leds[mid_point - led] = CHSV(2*hue, 255, 3*brit);
  }
  fadeToBlackBy(leds, NUM_LEDS, 50);

  #ifdef DEBUG
    Serial.print("\t Red: ");
    Serial.print(red);
    Serial.print("\t Green: ");
    Serial.print(green);
    Serial.print("\t Blue: ");
    Serial.print(blue);
    Serial.print("\t Hue: ");
    Serial.print(hue);
    Serial.print("\t vbrightness: ");
    Serial.println(brit);
  #endif
}

void band_brightness() {
  double *fiveSamples = band_split_bounce();
  #ifdef DEBUG
    Serial.print("Vol1:");
    Serial.print(fiveSamples[0]);
    Serial.print("\tVol2:");
    Serial.print(fiveSamples[1]);
    Serial.print("\tVol3:");
    Serial.print(fiveSamples[2]);
    Serial.print("\tVol4:");
    Serial.print(fiveSamples[3]);
    Serial.print("\tVol5:");
    Serial.print(fiveSamples[4]);
  #endif
  for (int i = 0; i < NUM_LEDS/5; i++) {
    leds[i] = CHSV(0,255, map(fiveSamples[0], 0, 5, 0, 255));
  }
  for (int i = NUM_LEDS/5; i < 2*NUM_LEDS/5; i++) {
    leds[i] = CHSV(60,255, map(fiveSamples[1], 0, 5, 0, 255));
  }
  for (int i = 2*NUM_LEDS/5; i < 3*NUM_LEDS/5; i++) {
    leds[i] = CHSV(100,255, map(fiveSamples[2], 0, 5, 0, 255));
  }
  for (int i = 3*NUM_LEDS/5; i < 4*NUM_LEDS/5; i++) {
    leds[i] = CHSV(160,255, map(fiveSamples[3], 0, 5, 0, 255));
  }
  for (int i = 4*NUM_LEDS/5; i < NUM_LEDS; i++) {
    leds[i] = CHSV(205,255, map(fiveSamples[4], 0, 5, 0, 255));
  }

  fadeToBlackBy(leds, NUM_LEDS, 10);

  delete [] fiveSamples;
}

void advanced_bands() {
  double avg1 = 0;
  double avg2 = 0;
  double avg3 = 0;
  double avg4 = 0;
  double avg5 = 0;

  fadeToBlackBy(leds, NUM_LEDS, 50);

  for (int i = 0; i < advanced_size; i++) {
    avg1 += max1[i];    
  }
  avg1 /= advanced_size;
  for (int i = 0; i < advanced_size; i++) {
    avg2 += max2[i];
  }
  avg2 /= advanced_size;
  for (int i = 0; i < advanced_size; i++) {
    avg3 += max3[i];
  }
  avg3 /= advanced_size;
  for (int i = 0; i < advanced_size; i++) {
    avg4 += max4[i];
  }
  avg4 /= advanced_size;
  for (int i = 0; i < advanced_size; i++) {
    avg5 += max5[i];
  }
  avg5 /= advanced_size;

  double *fiveSamples = band_split_bounce();

  double vol1 = fiveSamples[0];
  double vol2 = fiveSamples[1];
  double vol3 = fiveSamples[2];
  double vol4 = fiveSamples[3];
  double vol5 = fiveSamples[4];

  #ifdef DEBUG
    Serial.print("ADVANCED::\tAVG1:\t");
    Serial.print(avg1);
    Serial.print("\tAVG2:\t");
    Serial.print(avg2);
    Serial.print("\tAVG3:\t");
    Serial.print(avg3);
    Serial.print("\tAVG4:\t");
    Serial.print(avg4);
    Serial.print("\tAVG5:\t");
    Serial.print(avg5);
  #endif

  if (vol1 <= avg1) {
    max1[maxIter] = 0;
  }
  else {
      for (int i = 0; i < 5; i++) {
        max1[i] = vol1;
      }
  }
    
  if (vol2 <= avg2) {
    max2[maxIter] = 0;
  }
  else {
      for (int i = 0; i < 5; i++) {
        max2[i] = vol2;
      }
  }

  if (vol3 <= avg3) {
    max3[maxIter] = 0;
  }
  else {
      for (int i = 0; i < 5; i++) {
        max3[i] = vol3;
      }
  }

  if (vol4 <= avg4) {
    max4[maxIter] = 0;
  }
  else {
      for (int i = 0; i < 5; i++) {
        max4[i] = vol4;
      }
  }

  if (vol5 <= avg5) {
    max5[maxIter] = 0;
  }
  else {
      for (int i = 0; i < 5; i++) {
        max5[i] = vol5;
      }
  }


  if (maxIter == advanced_size-1) {
    maxIter = 0;
  } else {
    maxIter++;
  }

  leds[(int) avg1] = CRGB(255,0,0);
  leds[(int) NUM_LEDS/5 + (int) avg2] = CRGB(255,255,0);
  leds[(int) 2*NUM_LEDS/5+(int) avg3] = CRGB(0,255,0);
  leds[(int) 3*NUM_LEDS/5+(int) avg4] = CRGB(0,255,255);
  leds[(int) 4*NUM_LEDS/5+(int) avg5] = CRGB(0,0,255);


  for (int i = 0; i < vol1-1; i++) {
    leds[i] = CRGB(255,0,0);
  }
  for (int i = NUM_LEDS/5; i < NUM_LEDS/5+vol2-1; i++) {
    leds[i] = CRGB(255,255,0);
  }
  for (int i = 2*NUM_LEDS/5; i < 2*NUM_LEDS/5+vol3-1; i++) {
    leds[i] = CRGB(0,255,0);
  }
  for (int i = 3*NUM_LEDS/5; i < 3*NUM_LEDS/5+vol4-1; i++) {
    leds[i] = CRGB(0,255,255);
  }
  for (int i = 4*NUM_LEDS/5; i < 4*NUM_LEDS/5+vol5-1; i++) {
    leds[i] = CRGB(0,0,255);
  }

  delete [] fiveSamples;
}

void basic_bands() {
  fadeToBlackBy(leds, NUM_LEDS, 85);

    // double *fiveSamples = band_sample_bounce();
    double *fiveSamples = band_split_bounce();

    double vol1 = fiveSamples[0];
    double vol2 = fiveSamples[1];
    double vol3 = fiveSamples[2];
    double vol4 = fiveSamples[3];
    double vol5 = fiveSamples[4];

    for (int i = 0; i < vol1; i++) {
      leds[i] = CRGB(255,0,0);
    }
    for (int i = NUM_LEDS/5; i < NUM_LEDS/5+vol2; i++) {
      leds[i] = CRGB(255,255,0);
    }
    for (int i = 2*NUM_LEDS/5; i < 2*NUM_LEDS/5+vol3; i++) {
      leds[i] = CRGB(0,255,0);
    }
    for (int i = 3*NUM_LEDS/5; i < 3*NUM_LEDS/5+vol4; i++) {
      leds[i] = CRGB(0,255,255);
    }
    for (int i = 4*NUM_LEDS/5; i < 4*NUM_LEDS/5+vol5; i++) {
      leds[i] = CRGB(0,0,255);
    }

    delete [] fiveSamples;
}

void eq() {
  blank();
  for (int i = 0; i < NUM_LEDS; i++) {
    int brit = map(vReal[i], MIN_FREQUENCY, MAX_FREQUENCY, 0, 255);
    int hue = map(i, 0, NUM_LEDS, 0, 255);
    if (vReal[i] > 200) {
      leds[i] = CHSV(hue, 255, brit);
    }
  }
}

void show_drums() {
  int* drums = drum_identify();
  fadeToBlackBy(leds, NUM_LEDS, 50);

  if (drums[0]) {
    fill_solid(leds, NUM_LEDS, CRGB(255, 0, 0));
  }
  if (drums[1]) {
    fill_solid(leds, 2*NUM_LEDS/3, CRGB(0, 255, 0));
  }
  if (drums[2]) {
    fill_solid(leds, NUM_LEDS/3, CRGB(0, 0, 255));
  }

  delete [] drums;
}

void show_formants() {
  double *temp_formants = density_formant();
  blank();
  fill_solid(leds, NUM_LEDS, CRGB(remap(temp_formants[0], 0, SAMPLES, log(1),50), remap(temp_formants[1], 0, SAMPLES, log(1),50), remap(temp_formants[2], 0, SAMPLES, log(1),50)));
  delete [] temp_formants;
}

void noisy() {
  if (nvp() == 1) {
    fill_solid(leds, NUM_LEDS, CRGB(255, 0, 0));
  }
  else {
    blank();
  }  
  #ifdef DEBUG
    Serial.print("\t CheckVol: ");
  #endif
}

void formant_band() {
  double *temp_formants = density_formant();
  double f0Hue = remap(temp_formants[0], MIN_FREQUENCY, MAX_FREQUENCY, 0, 255);
  double f1Hue = remap(temp_formants[1], MIN_FREQUENCY, MAX_FREQUENCY, 0, 255);
  double f2Hue = remap(temp_formants[2], MIN_FREQUENCY, MAX_FREQUENCY, 0, 255);

  #ifdef DEBUG
    Serial.print("\t f0Hue: ");
    Serial.print(temp_formants[0]);
    Serial.print("\t f1Hue: ");
    Serial.print(temp_formants[1]);
    Serial.print("\t f2Hue: ");
    Serial.print(temp_formants[2]);
  #endif

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < NUM_LEDS/3) {
      leds[i] = CHSV(f0Hue, 255, 255);
    }
    else if (NUM_LEDS/3 <= i && i < 2*NUM_LEDS/3) {
      leds[i] = CHSV(f1Hue, 255, 255);
    } 
    else {
      leds[i] = CHSV(f2Hue, 255, 255);
    }
  }

  for (int i = 0; i < 5; i++) {
    blur1d(leds, NUM_LEDS, 50);
  }
  delete[] temp_formants;
}

void alt_drums() {
  int len = (sizeof(vReal)/sizeof(vReal[0])) / 7;
  double smol_arr[len];
  memcpy(smol_arr, vReal, len-1);
  double F0 = FFT.MajorPeak(smol_arr, SAMPLES, SAMPLING_FREQUENCY);
  memcpy(smol_arr, vReal + (3*len), len-1);
  double F1 = FFT.MajorPeak(smol_arr, SAMPLES, SAMPLING_FREQUENCY);
  memcpy(smol_arr, vReal + (5*len), len-1);
  double F2 = FFT.MajorPeak(smol_arr, SAMPLES, SAMPLING_FREQUENCY);


  double fHue0 = map(log ( F0/7 ) / log ( 2 ), log ( MIN_FREQUENCY ) / log ( 2 ), log ( MAX_FREQUENCY ) / log ( 2 ), 0, 240);
  double fHue1 = map(log ( F1/7 ) / log ( 2 ), log ( MIN_FREQUENCY ) / log ( 2 ), log ( MAX_FREQUENCY ) / log ( 2 ), 0, 240);
  double fHue2 = map(log ( F2/7 ) / log ( 2 ), log ( MIN_FREQUENCY ) / log ( 2 ), log ( MAX_FREQUENCY ) / log ( 2 ), 0, 240);

  #ifdef DEBUG
    Serial.print("\t pattern: Alt Drums\t \nfHue0: ");
    Serial.print(F0/7);
    Serial.print("\t fHue1: ");
    Serial.println(F1/7);
    Serial.print("\t fHue2: ");
    Serial.println(F2/7);
    Serial.print("\t vbrightness: ");
    Serial.println(vbrightness);
  #endif
  
  CHSV color = CHSV( fHue1, 255, vbrightness);
  fill_solid(leds, NUM_LEDS, color);
}

void formant_test(){
    int len = (sizeof(vReal)/sizeof(vReal[0])) / 5;
    double smol_arr[len];
    memcpy(smol_arr, vReal, len-1);
    int F0 = largest(smol_arr, len);
    memcpy(smol_arr, vReal + len, len-1);
    int F1 = largest(smol_arr, len);
    memcpy(smol_arr, vReal + (2*len), len-1);
    int F2 = largest(smol_arr, len);

    #ifdef DEBUG
      Serial.print("\t pattern: Formant Test\t fHue: ");
      Serial.print(fHue);
      Serial.print("\t vbrightness: ");
      Serial.println(vbrightness);
    #endif
   
    leds[0] = CHSV( fHue, F0, vbrightness);
    leds[1] = CHSV( fHue, F1, vbrightness);
    CRGB temp;
    
    for(int i = NUM_LEDS-1; i > 1; i-=2) {
        leds[i] = leds[i-2];
        leds[i-1] = leds[i-2];
    } 
}

void Fire2012WithPalette(){
  
  int sparkVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 10,200);
  //int coolingVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 60, 40);
  //Serial.println(sparkVolume);
  
  
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < sparkVolume ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    //Step 3.5. Calcualate Brightness from low frequencies
    int len = (sizeof(vReal)/sizeof(vReal[0])) / 7;
    double smol_arr[len];
    memcpy(smol_arr, vReal, len-1);
      
    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
    
      byte colorindex = scale8( heat[j], 240);
      //byte colorindex = scale8( heat[j], smol_arr);
      CRGB color = ColorFromPalette( gPal, colorindex);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
    }
}
//adapted from s-marley
void saturated_noise(){

  //Set params for fill_noise16()
  uint8_t octaves = 1;
  uint16_t x = 0;
  int scale = 300;
  uint8_t hue_octaves = 1;
  uint16_t hue_x = 100;
  int hue_scale = 20; 
  uint16_t ntime = millis() / 3;
  uint8_t hue_shift =  50;

  //Fill LEDS with noise using parameters above
  fill_noise16 (leds, NUM_LEDS, octaves, x, scale, hue_octaves, hue_x, hue_scale, ntime, hue_shift);
  //Add blur
  blur1d(leds, NUM_LEDS, 80);
}

void saturated_noise_hue_octaves(){

  //Remap volume variable that will change hue_octaves below
  int hueOctavesFromVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 1, 10);
  
  //Set params for fill_noise16()
  uint8_t octaves = 1;
  uint16_t x = 0;
  int scale = 300;
  uint8_t hue_octaves = hueOctavesFromVolume;
  uint16_t hue_x = 100;
  int hue_scale = 20; 
  uint16_t ntime = millis() / 3;
  uint8_t hue_shift =  50;
  
  //Fill LEDS with noise using parameters above
  fill_noise16 (leds, NUM_LEDS, octaves, x, scale, hue_octaves, hue_x, hue_scale, ntime, hue_shift);
  //Add blur
  blur1d(leds, NUM_LEDS, 80); 
}

void saturated_noise_hue_shift(){

  //Remap volume variable that will change octaves and hue_shift below
  int shiftFromVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 50, 100);

  //Set params for fill_noise16()
  uint8_t octaves = shiftFromVolume;
  uint16_t x = 0;
  int scale = 230;
  uint8_t hue_octaves = 1;
  uint16_t hue_x = 150;
  int hue_scale = 20; //prev 50
  uint16_t ntime = millis() / 3;
  uint8_t hue_shift =  shiftFromVolume;

  //Fill LEDS with noise using parameters above
  fill_noise16 (leds, NUM_LEDS, octaves, x, scale, hue_octaves, hue_x, hue_scale, ntime, hue_shift);
  //Add blur
  blur1d(leds, NUM_LEDS, 80);
}

void saturated_noise_compression(){

  //Remap volume variable that will change hue_x below
  int shiftFromVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 1, 8);

  //Set params for fill_noise16()
  uint8_t octaves = 1;
  uint16_t x = 0;
  int scale = 300;
  uint8_t hue_octaves = 1;
  uint16_t hue_x = shiftFromVolume;
  int hue_scale = 20; //prev 50
  uint16_t ntime = millis() / 4;
  uint8_t hue_shift =  50;

  //Fill LEDS with noise using parameters above
  fill_noise16 (leds, NUM_LEDS, octaves, x, scale, hue_octaves, hue_x, hue_scale, ntime, hue_shift);
  //Add blur
  blur1d(leds, NUM_LEDS, 80);
}

void groovy_noise(){
  
  //setup parameters for fill_noise16()
  uint8_t octaves = 1;
  uint16_t x = 0;
  int scale = 100;
  uint8_t hue_octaves = 1;
  uint16_t hue_x = 1;
  int hue_scale = 50;
  uint16_t ntime = millis() / 3;
  uint8_t hue_shift =  5;
  
  //Fill LEDS with noise using parameters above
  fill_noise16 (leds, NUM_LEDS, octaves, x, scale, hue_octaves, hue_x, hue_scale, ntime, hue_shift);
  
  //Add blur
  blur1d(leds, NUM_LEDS, 80); 
}

void groovy_noise_hue_shift_change(){

  // 210-230 is good range for the last parameter in shiftFromVolume
  int shiftFromVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 5, 220);
  int xFromVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 1, 2);

  //Setup parameters for fill_noise16()
  uint8_t octaves = 1;
  uint16_t x = 0;
  int scale = 100;
  uint8_t hue_octaves = 1;
  uint16_t hue_x = xFromVolume;
  int hue_scale = 20;
  uint16_t ntime = millis() / 3;
  uint8_t hue_shift =  shiftFromVolume;

  //Fill LEDS with noise using parameters above
  fill_noise16 (leds, NUM_LEDS, octaves, x, scale, hue_octaves, hue_x, hue_scale, ntime, hue_shift);
  //Blur for smoother look
  blur1d(leds, NUM_LEDS, 80); 
}

void sin_hue_trail(){

  //Create sin beat
  uint16_t sinBeat0  = beatsin16(12, 0, NUM_LEDS-1, 0, 0);
  
  //Given the sinBeat and fHue, color the LEDS and fade
  leds[sinBeat0]  = CHSV(fHue, 255, MAX_BRIGHTNESS);
  fadeToBlackBy(leds, NUM_LEDS, 5);
}

//Frequency hue trail that expands outward from the center. Adjusted from legacy freq_hue_trail
void freq_hue_trail_mid(){
    
  //color middle LEDs based on fHue
  leds[NUM_LEDS/2-1] = CHSV( fHue, 255, vbrightness);
  leds[NUM_LEDS/2] = CHSV( fHue, 255, vbrightness);
    
  //Move LEDS outward from the middle (only on one half)
  for(int i = NUM_LEDS-1; i > NUM_LEDS/2; i-=2) {
    leds[i] = leds[i-2];
    leds[i-1] = leds[i-2];
  }

  //Mirror LEDS from one half to the other half
  for(int i = 0; i < NUM_LEDS/2; ++i){
    leds[NUM_LEDS/2-i] = leds[NUM_LEDS/2+i];
  }
}

void freq_hue_trail_mid_blur(){
    
  //color middle LEDs based on fHue
  leds[NUM_LEDS/2-1] = CHSV( fHue, 255, vbrightness);
  leds[NUM_LEDS/2] = CHSV( fHue, 255, vbrightness);
    
  //Move LEDS outward from the middle (only on one half)
  for(int i = NUM_LEDS-1; i > NUM_LEDS/2; i-=2) {
    leds[i] = leds[i-2];
    leds[i-1] = leds[i-2];
  }

  //Mirror LEDS from one half to the other half
  for(int i = 0; i < NUM_LEDS/2; ++i){
    leds[NUM_LEDS/2-i] = leds[NUM_LEDS/2+i];
  }
  
  //Add blur
  blur1d(leds, NUM_LEDS, 20); 
}

void talking_hue(){

  //remap the volume variable to move the LEDS from the middle to the outside of the strip
  int offsetFromVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 1, NUM_LEDS/2);
  int midpoint = NUM_LEDS/2;
  
  //3 LED groups. One stationary in the middle, two that move outwards based on volume
  leds[midpoint]                   = CHSV(fHue/2, 255, MAX_BRIGHTNESS);
  leds[midpoint-offsetFromVolume]  = CHSV(fHue, 255,   MAX_BRIGHTNESS);
  leds[midpoint+offsetFromVolume]  = CHSV(fHue, 255,   MAX_BRIGHTNESS);

  //Add blur and quick fade
  blur1d(leds, NUM_LEDS, 80);
  fadeToBlackBy(leds, NUM_LEDS, 150);
}

void talking_formants(){

  //Use formant analysis
  double *formants = density_formant();
  double f0Hue = remap(formants[0], MIN_FREQUENCY, MAX_FREQUENCY, 0, 255);
  double f1Hue = remap(formants[1], MIN_FREQUENCY, MAX_FREQUENCY, 0, 255);
  double f0 = remap(formants[0], MIN_FREQUENCY, MAX_FREQUENCY, 0, 255);
  double f1 = remap(formants[1], MIN_FREQUENCY, MAX_FREQUENCY, 0, 255);
  double f2 = remap(formants[2], MIN_FREQUENCY, MAX_FREQUENCY, 0, 255);
  
  //remap the volume variable to move the LEDS from the middle to the outside of the strip
  int offsetFromVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 1, 30);
 
  //3 LED groups. One stationary in the middle, two that move outwards 
  leds[NUM_LEDS/2]                   = CRGB(f0, f1, f2);
  leds[NUM_LEDS/2-offsetFromVolume]  = CHSV(f0Hue, 255, MAX_BRIGHTNESS);
  leds[NUM_LEDS/2+offsetFromVolume]  = CHSV(f0Hue, 255, MAX_BRIGHTNESS);

  //blur and fade
  blur1d(leds, NUM_LEDS, 80);
  fadeToBlackBy(leds, NUM_LEDS, 200);
  
  //reset formant array for next loop
  delete[] formants;
}

void talking_moving(){
  
  //Last var good range (7500-12500)
  int offsetFromVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 0, 12500);

  //Create 3 sin beats with the offset(last parameter) changing based on offsetFromVolume
  uint16_t sinBeat0   = beatsin16(5, 2, NUM_LEDS-3, 0, 250);
  uint16_t sinBeat1  = beatsin16(5, 2, NUM_LEDS-3, 0, 0 - offsetFromVolume);
  uint16_t sinBeat2  = beatsin16(5, 2, NUM_LEDS-3, 0, 750 + offsetFromVolume);

  //Given the sinBeats and fHue, color the LEDS
  leds[sinBeat0]  = CHSV(fHue+100, 255, MAX_BRIGHTNESS);
  leds[sinBeat1]  = CHSV(fHue, 255, MAX_BRIGHTNESS);
  leds[sinBeat2]  = CHSV(fHue, 255, MAX_BRIGHTNESS);

  //Add blur and fade
  blur1d(leds, NUM_LEDS, 80);
  fadeToBlackBy(leds, NUM_LEDS, 100);
}

void bounce_back(){
  
  //Last var good range (7500-12500)
  int offsetFromVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 0, 12500);

  //Create 2 sinBeats with the offset(last parameter) of sinBeat2 changing based on offsetFromVolume
  uint16_t sinBeat   = beatsin16(6, 2, NUM_LEDS-3, 0, 500);
  uint16_t sinBeat2  = beatsin16(6, 2, NUM_LEDS-3, 0, 0 - offsetFromVolume);

  //Given the sinBeats and fHue, color the LEDS
  leds[sinBeat]   = CHSV(fHue-25, 255, MAX_BRIGHTNESS);
  leds[sinBeat2]  = CHSV(fHue, 255, MAX_BRIGHTNESS);
  
  //Add fade and blur
  blur1d(leds, NUM_LEDS, 80);
  fadeToBlackBy(leds, NUM_LEDS, 100);
}

void glitch(){
  //Remap the volume variable. Adjust the last parameter for different effects
  //Good range is 15-30
  //Crazy is 100-1000
  //Super crazy is 1000+
  int speedFromVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 5, 25);

  //Create 2 sin waves(sinBeat, sinBeat2) that mirror eachother by default when no music is played
  //Use speedFromVolume variable assigned above as the beatsin16 speed parameter
  uint16_t sinBeat0   = beatsin16(speedFromVolume, 0, NUM_LEDS-1, 0, 0);
  uint16_t sinBeat1  = beatsin16(speedFromVolume, 0, NUM_LEDS-1, 0, 32767);

  //Use formant analysis
  double *formants = density_formant();
  double f0Hue = remap(formants[0], MIN_FREQUENCY, MAX_FREQUENCY, 0, 255);
  
  //Given the sinBeats above and f0Hue, color the LEDS
  leds[sinBeat0]  = CHSV(fHue, 255, MAX_BRIGHTNESS);
  leds[sinBeat1]  = CHSV(f0Hue, 255, MAX_BRIGHTNESS); //can use fHue instead of formants

  //add blur and fade  
  blur1d(leds, NUM_LEDS, 80);
  fadeToBlackBy(leds, NUM_LEDS, 40);
}

void glitch_talk(){
  
  //Good values for the last paramter below = [7500,12500,20000]
  int offsetFromVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 0, 20000);
  int speedFromVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 5, 20);

  //Create 3 sin beats with the speed and offset(first and last parameters) changing based off variables above
  uint16_t sinBeat0  = beatsin16(speedFromVolume, 3, NUM_LEDS-4, 0, 250);
  uint16_t sinBeat1  = beatsin16(speedFromVolume, 3, NUM_LEDS-4, 0, 0 - offsetFromVolume);
  uint16_t sinBeat2  = beatsin16(speedFromVolume, 3, NUM_LEDS-4, 0, 750 + offsetFromVolume);

  //Given the sinBeats and fHue, color the LEDS  
  leds[sinBeat0]  = CHSV(fHue*2, 255, MAX_BRIGHTNESS);
  leds[sinBeat1]  = CHSV(fHue, 255, MAX_BRIGHTNESS);
  leds[sinBeat2]  = CHSV(fHue, 255, MAX_BRIGHTNESS);

  //Add blur and fade
  blur1d(leds, NUM_LEDS, 80);
  fadeToBlackBy(leds, NUM_LEDS, 100); 
}

void glitch_sections(){

  //Last var good range (5000-10000)
  int offsetFromVolume = remap(volume, MIN_VOLUME, MAX_VOLUME, 0, 10000);

  //Create 4 sin beats with the offset(last parameter) changing based off offsetFromVolume
  uint16_t sinBeat0  = beatsin16(6, 0, NUM_LEDS-1, 0, 0     - offsetFromVolume);
  uint16_t sinBeat1  = beatsin16(6, 0, NUM_LEDS-1, 0, 16384 - offsetFromVolume);
  uint16_t sinBeat2  = beatsin16(6, 0, NUM_LEDS-1, 0, 32767 - offsetFromVolume);
  uint16_t sinBeat3  = beatsin16(6, 0, NUM_LEDS-1, 0, 49151 - offsetFromVolume);

  //Given the sinBeats and fHue, color the LEDS
  leds[sinBeat0]  = CHSV(fHue, 255, MAX_BRIGHTNESS);
  leds[sinBeat1]  = CHSV(fHue, 255, MAX_BRIGHTNESS);
  leds[sinBeat2]  = CHSV(fHue, 255, MAX_BRIGHTNESS);
  leds[sinBeat3]  = CHSV(fHue, 255, MAX_BRIGHTNESS);
 
  //Add blur and fade 
  blur1d(leds, NUM_LEDS, 80);
  fadeToBlackBy(leds, NUM_LEDS, 60);
}

void volume_level_middle_bar_freq_hue_with_fade_and_blur(){
    #ifdef DEBUG   
      Serial.print("\t pattern: volume_level_middle_bar_freq_hue\t volume: ");
      Serial.print(volume);
      Serial.print("\t peak: ");
      Serial.println(peak);
    #endif

    int n = remap(volume, MIN_VOLUME, MAX_VOLUME, 0, NUM_LEDS/2);
    int mid_point = (int) NUM_LEDS/2;
    
    for(int led = 0; led < n; led++) {
              leds[mid_point + led].setHue( fHue );
              leds[mid_point - led].setHue( fHue );
    }
    
    blur1d(leds, NUM_LEDS, 80); 
    fadeToBlackBy( leds, NUM_LEDS, 20);
}