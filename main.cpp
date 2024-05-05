#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <chrono>
#include <thread>

#include <SDL2/SDL.h>

#include "cpu.cpp"

SDL_Renderer* renderer;
SDL_Window* window;
SDL_Texture* texture;

const int keymap[16] = {
    SDL_SCANCODE_X,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_W,
    SDL_SCANCODE_E,
    SDL_SCANCODE_A,
    SDL_SCANCODE_S,
    SDL_SCANCODE_D,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_C,
    SDL_SCANCODE_4,
    SDL_SCANCODE_R,
    SDL_SCANCODE_F,
    SDL_SCANCODE_V
};

Cpu cpu;

void init() {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) { throw std::runtime_error("SDL INITALIZATION FAILED."); }

    // window with 64x32 ratio
    window = SDL_CreateWindow("eightmulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 512, SDL_WINDOW_VULKAN);

    if (window == NULL) { throw std::runtime_error("WINDOW CREATION FAILED."); }

    renderer = SDL_CreateRenderer(window, -1, 0);

    if (renderer == NULL) { throw std::runtime_error("RENDERER CREATION FAILED."); }

    // 64 x 32 texture
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
    if (texture == NULL) { throw std::runtime_error("TEXTURE CREATION FAILED."); }
}

void deinit() {
    //SDL_DestroyWindow(window);
    SDL_Quit();
}

void loadROM(char *filename) {
   std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (file.is_open()) {
        char c;
        int check = 0x200;
        for (int i = 0x200; file.get(c); i++) {
            if (check >= 4096) {
                throw std::runtime_error("ROM TOO LARGE.");
            }
            cpu.memory[i] = (uint8_t) c;
            check++;
        }
    }
}

int main(int argc, char *argv[]) {
    try {
        init();

        cpu.init(true);

        char* filename = argv[1];

        if (filename == NULL) {
            printf("No ROM argument present.\n");
            exit(-1);
        }

        loadROM(filename);

        // Main loop
        bool keepOpen = true;
        while (keepOpen) {
            cpu.cycle();

            // SDL Events
            SDL_Event e;
            while(SDL_PollEvent(&e) > 0) {
                switch (e.type) {
                    case SDL_QUIT: 
                        keepOpen = false;
                        break;
                    case SDL_KEYDOWN:
                        for(int i = 0; i < 16; i++) {
                            if (e.key.keysym.scancode == keymap[i]) {
                                cpu.keys[i] = 1;
                            }
                        }
                        break;
                    case SDL_KEYUP:
                        for(int i = 0; i < 16; i++) {
                            if (e.key.keysym.scancode == keymap[i]) {
                                cpu.keys[i] = 0;
                            }
                        }
                }
            }

            SDL_RenderClear(renderer);

            uint32_t* bytes = new uint32_t[64 * 32];
            int pitch = 0;

            SDL_LockTexture(texture, NULL, (void**) &bytes, &pitch);

            for (int y = 0; y < 32; y++) {
                for (int x  = 0; x < 64; x++) {
                    bytes[y * 64 + x] = (cpu.graphics[y * 64 + x] == 1) ? 0xFFFFFF : (uint32_t) 0x000000;
                }
            }

            SDL_UnlockTexture(texture);

            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);

            // sleep for 16 milliseconds for 16Hz
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        // Close
        deinit();
        exit(0);

    } catch(const std::runtime_error& error) {
        std::cout << error.what() << "\n";
        deinit();
        exit(-1);
    } catch (...) {
        std::cout << "an error ocurred." << "\n";
        deinit();
        exit(-1);
    }
}
