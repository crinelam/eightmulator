#include <SDL2/SDL.h>
#include <iostream>
#include <string>
#include <math.h>

class Audio {
    private:
        static SDL_AudioDeviceID device;

        static double frequency;
        static double volume;

        static int pos;
        
        static int (*calcOffset)(int sample, int channel);
        static void (*writeData)(uint8_t* ptr, double data);

        static double getData();

        static void callback(void *userdata, Uint8 *stream, int len);

    public:
        static void open();
        static void close();

        static void setFreq(double freq);
        static void setVol(double vol);

        static void play();
        static void stop();

        static SDL_AudioSpec spec;
};