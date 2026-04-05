//
// Created by Zelgius on 20-03-26.
//

#include <ezBuzzer.h>
#include <buzzer/task_buzzer.h>
#include <logger/task_logger.h>

ezBuzzer buzzer(BUZZER_PIN, BUZZER_TYPE_PASSIVE, HIGH); // create ezBuzzer object: pin, type, activeLevel (PASSIVE for melody)

extern QueueHandle_t buzzerQueue;

#define REST 0

// notes in the melody:
int melody[] = {
    NOTE_E5, 8, NOTE_E5, 8, REST, 8, NOTE_E5, 8, REST, 8, NOTE_C5, 8, NOTE_E5, 8, //1
    NOTE_G5, 4, REST, 4, NOTE_G4, 8, REST, 4,
};

int tempo = 200;

int fireballMelody[] = {
    NOTE_G4, 64,
    NOTE_C5, 64,
    NOTE_G5, 64,
    NOTE_C6, 64
  };

// For this specific sound effect, a faster tempo works better
int fireballTempo = 240;

// Mario Coin Get Melody
// The coin sound is a quick B5 followed by a sustained E6
int coinMelody[] = {
    NOTE_B5, 16,  // Very short first note
    NOTE_E6, 4    // Longer, ringing second note
};

// High tempo for that "instant" arcade feel
int coinTempo = 180;


void playMelody(const int *melodyArray, int count, int tempoValue) {
    int notesCount = count / 2;
    int wholenote = (60000 * 4) / tempoValue;

    for (int thisNote = 0; thisNote < notesCount * 2; thisNote = thisNote + 2) {
        int note = melodyArray[thisNote];
        int divider = melodyArray[thisNote + 1];
        int noteDuration = 0;

        if (divider > 0) {
            noteDuration = (wholenote) / divider;
        } else if (divider < 0) {
            noteDuration = (wholenote) / abs(divider);
            noteDuration *= 1.5;
        }

        if (note != REST) {
            buzzer.beep(noteDuration * 0.9, 0, note);
        } else {
            vTaskDelay(pdMS_TO_TICKS(noteDuration));
        }

        while (buzzer.getState() != BUZZER_IDLE) {
            buzzer.loop();
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        vTaskDelay(pdMS_TO_TICKS(noteDuration * 0.1));
    }
}

[[noreturn]] void buzzerTask(void *pvParameters) {
    LogEvent::post("Buzzer task started\n");
    for (;;) {
        BuzzerEvent event{};

        if (buzzer.getState() == BUZZER_IDLE) {
            if (xQueueReceive(buzzerQueue, &event, pdMS_TO_TICKS(10))) {
                buzzer.turnON();
                if (event.type == BuzzerType::BIP) {
                    playMelody(fireballMelody, sizeof(fireballMelody) / sizeof(fireballMelody[0]), fireballTempo);
                } else if (event.type == BuzzerType::BIP2) {
                    playMelody(coinMelody, sizeof(coinMelody) / sizeof(coinMelody[0]), coinTempo);
                } else if (event.type == BuzzerType::MELODY) {
                    playMelody(melody, sizeof(melody) / sizeof(melody[0]), tempo);
                }
                buzzer.turnOFF();
            }
        }

        buzzer.loop();
        taskYIELD();
    }
}

bool BuzzerEvent::post(BuzzerType type, uint32_t frequency, uint32_t duration) {
    if (buzzerQueue == nullptr) return false;

    BuzzerEvent event{};
    event.type = type;
    event.frequency = frequency;
    event.duration = duration;

    if (xQueueSend(buzzerQueue, &event, pdMS_TO_TICKS(10)) != pdPASS) {
        LogEvent::post("[ERROR] Buzzer Queue Full!\n");
        return false;
    }
    return true;
}

void BuzzerEvent::bip(const uint32_t frequency, const uint32_t duration) {
    post(BuzzerType::BIP, frequency, duration);
}

void BuzzerEvent::bip2(const uint32_t frequency, const uint32_t duration) {
    post(BuzzerType::BIP2, frequency, duration);
}

void BuzzerEvent::melody() {
    post(BuzzerType::MELODY);
}

