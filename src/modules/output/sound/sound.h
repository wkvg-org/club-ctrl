/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Piano-note frequencies (equal temperament, A4 = 440 Hz), rounded to Hz.
 * Sharp = "S" suffix, flats aliased to sharps (D♭5 == C#5 == NOTE_DS5... err
 * == NOTE_CS5). NOTE_REST is silence. Range C3..C6 — plenty for a passive
 * piezo, and covers everything below that gets muddy anyway.
 */

#ifndef SOUND_H_
#define SOUND_H_

#include <stddef.h>
#include <stdint.h>

#define NOTE_REST 0

#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_DB3 NOTE_CS3
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_EB3 NOTE_DS3
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_GB3 NOTE_FS3
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_AB3 NOTE_GS3
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_BB3 NOTE_AS3
#define NOTE_B3  247

#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_DB4 NOTE_CS4
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_EB4 NOTE_DS4
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_GB4 NOTE_FS4
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_AB4 NOTE_GS4
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_BB4 NOTE_AS4
#define NOTE_B4  494

#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_DB5 NOTE_CS5
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_EB5 NOTE_DS5
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_GB5 NOTE_FS5
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_AB5 NOTE_GS5
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_BB5 NOTE_AS5
#define NOTE_B5  988

#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_DB6 NOTE_CS6
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_EB6 NOTE_DS6
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_GB6 NOTE_FS6
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_AB6 NOTE_GS6
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_BB6 NOTE_AS6
#define NOTE_B6  1976
#define NOTE_C7  2093

struct note {
	uint16_t freq_hz;
	uint16_t ms;
};

int  sound_init(void);
int  sound_play(const struct note *song, size_t n);
void sound_stop(void);

extern const struct note sandstorm[];
extern const size_t      sandstorm_len;

extern const struct note startup_chime[];
extern const size_t      startup_chime_len;

extern const struct note never_gonna[];
extern const size_t      never_gonna_len;

extern const struct note blue_dabadee[];
extern const size_t      blue_dabadee_len;

extern const struct note lamour_toujours[];
extern const size_t      lamour_toujours_len;

extern const struct note rhythm_is_a_dancer[];
extern const size_t      rhythm_is_a_dancer_len;

extern const struct note dota[];
extern const size_t      dota_len;

extern const struct note song[];
extern const size_t      song_len;

extern const struct note song[];
extern const size_t      song_len;

#endif /* SOUND_H_ */
