//
// Created by mastwet on 24-9-27.
//

#ifndef CHOCOLY_MIDI_H
#define CHOCOLY_MIDI_H

#include <vector>

class RPMidi {
public:
    RPMidi() {}
    void init(){
        //初始化外设
    }
    void MiDi_SendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity){

    }
    void MiDi_SendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity){

    }
    void MiDi_SendControlChange(uint8_t channel, uint8_t control, uint8_t value){

    }
    void MiDi_SendProgramChange(uint8_t channel, uint8_t program){

    }
private:

};

#endif //CHOCOLY_MIDI_H
