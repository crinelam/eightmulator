#include "audio.h"

SDL_AudioDeviceID Audio::device;
SDL_AudioSpec Audio::spec;

double Audio::frequency;
double Audio::volume;

int Audio::pos;

int (*Audio::calcOffset)(int sample, int channel);
void (*Audio::writeData)(uint8_t* ptr, double data);

int calcOffsets16(int sample, int channel) {
    return (sample * sizeof(int16_t) * Audio::spec.channels) + (channel * sizeof(int16_t));
}

int calcOffsetf32(int sample, int channel) {
    return (sample * sizeof(float) * Audio::spec.channels) + (channel * sizeof(float));
}

void writeDatas16(uint8_t* ptr, double data) {
    int16_t* ptrTyped = (int16_t*) ptr;

    double range = (double) INT16_MAX - (double) INT16_MIN;
    double dataScaled = data * range / 2.0;

    *ptrTyped = data;
}

void writeDataf32(uint8_t* ptr, double data) {
    float* ptrTyped = (float*) ptr;
    *ptrTyped = data;
}

double Audio::getData() {
    double sampleRate = (double) spec.freq;

    double period = sampleRate / frequency;

    if (pos % (int) period == 0) {
        pos = 0;
    }

    double angularFreq = (1.0 / period) * 2.0 * M_PI;
    double amplitude = volume;

    return sin((double) pos * angularFreq) * amplitude;
}

void Audio::callback(void* userdata, uint8_t* stream, int len) {
    for (int sample = 0; sample < spec.samples; ++sample) {
        double data = getData();
        pos++;

        for (int channel = 0; channel < spec.channels; ++channel) {
            int offset = calcOffset(sample, channel);
            uint8_t* ptrData = stream + offset;
            writeData(ptrData, data);
        }
    }

}

void Audio::open() {
    SDL_AudioSpec desired;
    SDL_zero(desired);

    desired.freq = 44100;
    desired.format = AUDIO_F32;
    desired.samples = 512;
    desired.channels = 1;
    desired.callback = Audio::callback;

    device = SDL_OpenAudioDevice(NULL, 0, &desired, &spec, 0);

    if (device == 0) {
        throw std::runtime_error("AUDIO DEVICE CREATION FAILED.");
    } else {
        std::string formatName;
        switch (spec.format) {
            case AUDIO_S16:
                writeData = writeDatas16;
                calcOffset = calcOffsets16;
                formatName = "AUDIO_S16";
                break;
            case AUDIO_F32:
                writeData = writeDataf32;
                calcOffset = calcOffsetf32;
                formatName = "AUDIO_F32";
                break;
            default:
                throw std::runtime_error("UNSUPPORTED AUDIO FORMAT.");
        }

        std::cout << "[Beeper] frequency: " << spec.freq << std::endl;
        std::cout << "[Beeper] format: " << formatName << std::endl;

        std::cout
            << "[Beeper] channels: "
            << (int)(spec.channels)
            << std::endl;

        std::cout << "[Beeper] samples: " << spec.samples << std::endl;
        std::cout << "[Beeper] padding: " << spec.padding << std::endl;
        std::cout << "[Beeper] size: " << spec.size << std::endl;
    }
}

void Audio::close() {
    SDL_CloseAudioDevice(device);
}

void Audio::setFreq(double freq) {
    frequency = freq;
}

void Audio::setVol(double vol) {
    volume = vol;
}

void Audio::play() {
    SDL_PauseAudioDevice(device, 0);
}

void Audio::stop() {
    SDL_PauseAudioDevice(device, 1);
}