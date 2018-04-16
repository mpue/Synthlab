/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   adsr_txt;
    const int            adsr_txtSize = 1205;

    extern const char*   highpass_txt;
    const int            highpass_txtSize = 644;

    extern const char*   lowpass_txt;
    const int            lowpass_txtSize = 1363;

    extern const char*   oscillator_txt;
    const int            oscillator_txtSize = 526;

    extern const char*   adder_png;
    const int            adder_pngSize = 3242;

    extern const char*   adsr_png;
    const int            adsr_pngSize = 2742;

    extern const char*   delete_png;
    const int            delete_pngSize = 3508;

    extern const char*   hp_png;
    const int            hp_pngSize = 2941;

    extern const char*   led_off_png;
    const int            led_off_pngSize = 5214;

    extern const char*   led_on_png;
    const int            led_on_pngSize = 11713;

    extern const char*   lp_png;
    const int            lp_pngSize = 2979;

    extern const char*   midiin_png;
    const int            midiin_pngSize = 4633;

    extern const char*   midiout_png;
    const int            midiout_pngSize = 4672;

    extern const char*   new_png;
    const int            new_pngSize = 3710;

    extern const char*   note_png;
    const int            note_pngSize = 2993;

    extern const char*   open_png;
    const int            open_pngSize = 3879;

    extern const char*   pulse_png;
    const int            pulse_pngSize = 1982;

    extern const char*   random_png;
    const int            random_pngSize = 1916;

    extern const char*   redo_png;
    const int            redo_pngSize = 3944;

    extern const char*   save_png;
    const int            save_pngSize = 3933;

    extern const char*   saw_png;
    const int            saw_pngSize = 2463;

    extern const char*   sawtooth_png;
    const int            sawtooth_pngSize = 5130;

    extern const char*   settings_png;
    const int            settings_pngSize = 4632;

    extern const char*   sine_png;
    const int            sine_pngSize = 2512;

    extern const char*   speaker_png;
    const int            speaker_pngSize = 4111;

    extern const char*   tri_png;
    const int            tri_pngSize = 2335;

    extern const char*   undo_png;
    const int            undo_pngSize = 4015;

    extern const char*   whitenoise_png;
    const int            whitenoise_pngSize = 7446;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Number of elements in the namedResourceList array.
    const int namedResourceListSize = 28;

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes) throw();
}
