#include "cpu.h"

void Cpu::init(bool print) {
    srand((uint32_t) time(0));

    // start of program memory 0x000 to 0x1FF reserved for interpreter mem
    programCounter = 0x200;

    opcode = 0;
    index = 0;
    stackPointer = 0;
    delayTimer = 0;
    soundTimer = 0;

    for (uint8_t x: memory) {
        x = 0;
    }
    for (uint8_t x: graphics) {
        x = 0;
    }
    for (uint8_t x: registers) {
        x = 0;
    }
    for (uint8_t x: stack) {
        x = 0;
    }
    for (uint8_t x: keys) {
        x = 0;
    }

    uint8_t i = 0;
    for (uint8_t c: fontset) {
        memory[i] = c;
        i++;
    }

    printInfo = print;

    audioPlaying = false;
    SDL_Init(SDL_INIT_AUDIO);
    audio.open();
    audio.setVol(0.25);
    audio.setFreq(392.00);
}

void Cpu::deinit() {
    audio.close();
}

void Cpu::incrementProgramCounter() {
    // incrementing by 2 cause every instruction is 2 bytes
    programCounter += 2;
}

void Cpu::cycle() {
    // read whole instruction from memory
    opcode = (memory[programCounter] << 8) | memory[programCounter+1];

    // get first nibble X000
    uint first = opcode >> 12;

    uint16_t sum;
    uint8_t Vx;
    uint8_t Vy;
    uint8_t kk; 
    uint8_t height;
    uint8_t regX;
    uint8_t regY;
    uint8_t mode;
    uint8_t pixel;

    if (printInfo) {
        printf("pc: %.4X opcode: %.4X sp: %.2X regs: ", programCounter, opcode, stackPointer);
        for (int i = 0; i < 15; i++) {
            printf("%.2X ", registers[i]);
        }
        printf("keys: ");
        for (int i = 0; i < 15; i++) {
            printf("%.2X ", keys[i]);
        }
        printf("delay: %X sound: %X", delayTimer, soundTimer);
        printf("\n");
    }
    

    switch(first) {
        case 0x0:
            if (opcode == 0x00E0) { // 00E0 - CLS; clear screen
                for(uint8_t g: graphics) {
                    g = 0;
                }
            } else if(opcode == 0x00EE) { // 00EE - RET; return from subrutine
                stackPointer--;
                programCounter = stack[stackPointer];
            } //else { } // 0nnn - SYS addr; system instruction, should be ignored
            incrementProgramCounter();
            break;
        case 0x1: // 1nnn - JP addr; Jump to location nnn
            programCounter = opcode & 0x0FFF;
            break;
        case 0x2: // 2nnn - CALL addr; Call subroutine at nnn
            stack[stackPointer] = programCounter;
            stackPointer++;
            programCounter = opcode & 0x0FFF;
            break;
        case 0x3: // 3xkk - SE Vx, byte; Skip next instruction if Vx == kkk
            Vx = (opcode & 0x0F00) >> 8;
            if (registers[Vx] == (opcode & 0x00FF)) {
                incrementProgramCounter();
            }
            incrementProgramCounter();
            break;
        case 0x4: // 4xkk - SNE Vx, byte; Skip next intruction if Vx != kk
            Vx = (opcode & 0x0F00) >> 8;
            if (registers[Vx] != (opcode & 0x00FF)) {
                incrementProgramCounter();
            }
            incrementProgramCounter();
            break;
        case 0x5: // 5xy0 - SE Vx, Vy; Skip next instruction if Vx == Vy
            Vx = (opcode & 0x0F00) >> 8;
            Vy = (opcode & 0x00F0) >> 4;
            if (registers[Vx] == registers[Vy]) {
                incrementProgramCounter();
            }
            incrementProgramCounter();
            break;
        case 0x6: // 6xkk - LD Vx, byte; Set Vx = kk
            Vx = (opcode & 0x0F00) >> 8;
            registers[Vx] = (opcode & 0x00FF);
            incrementProgramCounter();
            break;
        case 0x7: // 7xkk - ADD Vx, byte; Set Vx = Vx + kk
            Vx = (opcode & 0x0F00) >> 8;
            registers[Vx] += opcode & 0x00FF;
            incrementProgramCounter();
            break;
        case 0x8: // ALU instructions
            Vx = (opcode & 0x0F00) >> 8;
            Vy = (opcode & 0x00F0) >> 4;
            //get last nibble to check the mode of operation
            mode = opcode & 0x000F;
            switch(mode) {
                case 0x0: // 8xy0 - LD Vx, Vy; Set Vx = Vy
                    registers[Vx] = registers[Vy];
                    break;
                case 0x1: // 8xy1 - OR Vx, Vy; Set Vx = Vx | Vy
                    registers[Vx] |= registers[Vy];
                    break;
                case 0x2: // 8xy2 - AND Vx, Vy; Set Vx = Vx & Vy
                    registers[Vx] &= registers[Vy];
                    break;
                case 0x3: // 8xy3 - XOR Vx, Vy; Set Vx = Vx ^ Vy
                    registers[Vx] ^= registers[Vy];
                    break;
                case 0x4: // 8xy4 - ADD Vx, Vy; Set Vx = Vx + Vy, Set Vf = carry
                    // type promotion to catch overflows
                    sum = registers[Vx];
                    sum += registers[Vy];

                    registers[0xF] = (sum > 255) ? 1 : 0;
                    registers[Vx] = sum & 0x00FF;
                    break;
                case 0x5: // 8xy5 - SUB Vx, Vy; Set Vx = Vx - Vy, set VF = NOT borrow
                    registers[0xF] = (registers[Vx] > registers[Vy]) ? 1 : 0;
                    registers[Vx] -= registers[Vy];
                    break;
                case 0x6: // 8xy6 - SHR Vx {, Vy}; Set Vx = Vx SHR 1
                    registers[0xF] = registers[Vx] & 1;
                    registers[Vx] >>= 1;
                    break;
                case 0x7: // 8xy7 - SUBN Vx, Vy; Set Vx = Vy - Vx, set VF = NOT borrow
                    registers[0xF] = (registers[Vy] > registers[Vx]) ? 1 : 0;
                    registers[Vx] = registers[Vy] - registers[Vx];
                    break;
                case 0xE: // 8xyE - SHL Vx {, Vy}; Set Vx = Vx SHL 1
                    registers[0xF] = ((registers[Vx] & 0x80) != 0) ? 1 : 0;
                    registers[Vx] <<= 1;
            }
            incrementProgramCounter();
            break;
        case 0x9: // 9xy0 - SE Vx, Vy; Skip next instruction if Vx != Vy
            Vx = (opcode & 0x0F00) >> 8;
            Vy = (opcode & 0x00F0) >> 4;
            if ((uint8_t) registers[Vx] != (uint8_t) registers[Vy]) {
                printf("%X, %X, Equal.\n", registers[Vx], registers[Vy]);
                incrementProgramCounter();
            } else {
                printf("%X, %X, Not equal.\n", registers[Vx], registers[Vy]);
            }
            incrementProgramCounter();
            break;
        case 0xA: // Annn - LD I, addr; Set I = nnn
            index = opcode & 0x0FFF;
            incrementProgramCounter();
            break;
        case 0xB: // Bnnn - JP V0, addr; Jump to nnn + V0
            programCounter = (opcode & 0x0FFF) + (uint16_t) registers[0x0];
            break;
        case 0xC: // Cxkk - RND Vx, byte; Set Vx = random byte & kk
            Vx = (opcode & 0x0F00) >> 8;
            kk = opcode & 0x00FF;

            registers[Vx] = rand() & kk;
            incrementProgramCounter();
            break;
        case 0xD: // Dxyn - DRW Vx, Vy, nibble; Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
            registers[0xF] = 0;
            regX = registers[(opcode & 0x0F00) >> 8];
            regY = registers[(opcode & 0x00F0) >> 4];
            height = opcode & 0x000F;

            for (int i = 0; i < height; i++) {
                pixel = memory[index + i];

                for (int j = 0; j < 8; j++) {
                        const uint8_t msb = 0x80;
                        if ((pixel & (msb >> j)) != 0) {
                            uint8_t tx = (regX + j) % 64;
                            uint8_t ty = (regY + i) % 32;

                            uint16_t idx = tx + ty * 64;

                            graphics[idx] ^= 1;

                            if (graphics[idx] == 0) {
                                registers[0xF] = 1;
                            }
                        }
                }
            }
            incrementProgramCounter();
            break;
        case 0xE:
            Vx = (opcode & 0x0F00) >> 8;
            mode = opcode & 0x00FF;

            if(mode == 0x9E) { // Ex9E - SKP Vx; Skip next instruction if key with the value of Vx is pressed
                if(keys[registers[Vx]] == 1) {
                    incrementProgramCounter();
                }
            } else if (mode == 0xA1) { // ExA1 - SKNP Vx; Skip next instruction if key with the value of Vx is not pressed
                if(keys[registers[Vx]] != 1) {
                    incrementProgramCounter();
                }
            }
            incrementProgramCounter();
            break;
        case 0xF:
            Vx = (opcode & 0x0F00) >> 8;
            mode = opcode & 0x00FF;
            if (mode == 0x07) { // Fx07 - LD Vx, DT; Set Vx = delay timer value
                registers[Vx] = delayTimer;
            } else if (mode == 0x0A) { // Fx0A - LD Vx, K; Wait for a key press, store the value of the key in Vx
                bool keyPressed = false;
                
                uint8_t i = 0;
                for (uint8_t key: keys) {
                    if(key != 0) {
                        registers[Vx] = i;
                        keyPressed = true;
                        break;
                    }
                    i++;
                }

                if (!keyPressed) {
                    return;
                }
                incrementProgramCounter();
                break;
            } else if (mode == 0x15) { // Fx15 - LD DT, Vx; Set delay timer = Vx
                delayTimer = registers[Vx];
            } else if (mode == 0x18) { // Fx18 - LD ST, Vx; Set sound timer = Vx
                soundTimer = registers[Vx];
            } else if (mode == 0x1E) { // Fx1E - ADD I, Vx; Set I = I + Vx
                index += registers[Vx];
            } else if (mode == 0x29) { // Fx29 - LD F, Vx; Set I = location of sprite for digit Vx
                index = registers[Vx] * 0x5;
            } else if (mode == 0x33) { // Fx33 - LD B, Vx; Store BCD representation of Vx in memory locations I, I+1, and I+2
                memory[index] = registers[Vx] / 100;
                memory[index+1] = (registers[Vx] / 10) % 10;
                memory[index+2] = registers[Vx] % 10;
            } else if (mode == 0x55) { // Fx55 - LD [I], Vx; Store registers V0 through Vx in memory starting at location I
                for (uint8_t i = 0; i <= Vx; i++) {
                    memory[index + i] = registers[i];
                }
            } else if (mode == 0x65) { // Fx65 - LD Vx, [I]; Read registers V0 through Vx from memory starting at location I
                for (uint8_t i = 0; i <= Vx; i++) {
                    registers[i] = memory[index + i];
                }
            }
            incrementProgramCounter();
    }

    if (delayTimer > 0) {
        delayTimer--;
    }

    if (soundTimer > 0) {
        if (!audioPlaying) {
            audioPlaying = true;
            audio.play();
        }
        soundTimer--;
    } else if (audioPlaying) {
        audioPlaying = false;
        audio.stop();
    }
}