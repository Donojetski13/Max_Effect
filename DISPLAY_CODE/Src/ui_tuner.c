/*
 * ui_tuner.c
 *
 *  Created on: Nov 8, 2026
 *      Author: ewan
 */

#include "ui_tuner.h"
#include "tuner.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static lv_obj_t *tuner_screen = NULL;
static lv_obj_t *tuner_note_label = NULL;
static lv_obj_t *tuner_freq_label = NULL;
static lv_obj_t *tuner_status_label = NULL;
static lv_obj_t *tuner_needle = NULL;

void UI_Tuner_Init(void)
{
    tuner_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(tuner_screen, lv_color_hex(0x151D29), 0);
    lv_obj_set_scrollable(tuner_screen, false);

    // Title
    lv_obj_t *title = lv_label_create(tuner_screen);
    lv_label_set_text(title, "GUITAR TUNER");
    lv_obj_set_style_text_color(title, lv_color_hex(0x8582AB), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    // Large detected note
    tuner_note_label = lv_label_create(tuner_screen);
    lv_label_set_text(tuner_note_label, "--");
    lv_obj_set_style_text_color(tuner_note_label, lv_color_hex(0xC084FC), 0);
    lv_obj_set_style_text_font(tuner_note_label, &lv_font_montserrat_48, 0);

    lv_obj_align(tuner_note_label, LV_ALIGN_TOP_MID, 0, 45);

    // Frequency
    tuner_freq_label = lv_label_create(tuner_screen);
    lv_label_set_text(tuner_freq_label, "-- Hz");
    lv_obj_set_style_text_color(tuner_freq_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(tuner_freq_label, LV_ALIGN_TOP_MID, 0, 110);

    // Flat / Sharp labels
    lv_obj_t *flat_label = lv_label_create(tuner_screen);
    lv_label_set_text(flat_label, "FLAT");
    lv_obj_set_style_text_color(flat_label, lv_color_hex(0x8582AB), 0);
    lv_obj_align(flat_label, LV_ALIGN_TOP_MID, -85, 155);

    lv_obj_t *sharp_label = lv_label_create(tuner_screen);
    lv_label_set_text(sharp_label, "SHARP");
    lv_obj_set_style_text_color(sharp_label, lv_color_hex(0x8582AB), 0);
    lv_obj_align(sharp_label, LV_ALIGN_TOP_MID, 85, 155);

    // Horizontal tuning line
    lv_obj_t *track = lv_obj_create(tuner_screen);
    lv_obj_set_size(track, 190, 4);
    lv_obj_align(track, LV_ALIGN_TOP_MID, 0, 187);
    lv_obj_set_style_bg_color(track, lv_color_hex(0x384153), 0);
    lv_obj_set_style_border_width(track, 0, 0);
    lv_obj_set_style_pad_all(track, 0, 0);
    lv_obj_set_scrollable(track, false);

    // Center marker
    lv_obj_t *center = lv_obj_create(tuner_screen);
    lv_obj_set_size(center, 3, 25);
    lv_obj_align(center, LV_ALIGN_TOP_MID, 0, 176);
    lv_obj_set_style_bg_color(center, lv_color_hex(0x8582AB), 0);
    lv_obj_set_style_border_width(center, 0, 0);
    lv_obj_set_style_pad_all(center, 0, 0);

    // Moving needle
    tuner_needle = lv_obj_create(tuner_screen);
    lv_obj_set_size(tuner_needle, 5, 29);
    lv_obj_align(tuner_needle, LV_ALIGN_TOP_MID, 0, 174);
    lv_obj_set_style_bg_color(tuner_needle, lv_color_hex(0xC084FC), 0);
    lv_obj_set_style_border_width(tuner_needle, 0, 0);
    lv_obj_set_style_pad_all(tuner_needle, 0, 0);

    // Tuning status
    tuner_status_label = lv_label_create(tuner_screen);
    lv_label_set_text(tuner_status_label, "Waiting...");
    lv_obj_set_style_text_color(tuner_status_label, lv_color_hex(0x8582AB), 0);
    lv_obj_align(tuner_status_label, LV_ALIGN_TOP_MID, 0, 215);

    // Back
    lv_obj_t *footer = lv_label_create(tuner_screen);
    lv_label_set_text(footer, "Press to go back");
    lv_obj_set_style_text_color(footer, lv_color_hex(0x8582AB), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -8);
}

lv_obj_t *UI_Tuner_GetScreen(void)
{
    return tuner_screen;
}

void UI_Tuner_SetFrequency(double freq)
{
    if (tuner_note_label == NULL || tuner_freq_label == NULL)
        return;

    if (!(freq > 0.0 && freq <= 10000.0))
    {
        lv_label_set_text(tuner_note_label, "--");
        lv_label_set_text(tuner_freq_label, "-- Hz");
        lv_label_set_text(tuner_status_label, "Waiting...");
        lv_obj_align(tuner_needle, LV_ALIGN_TOP_MID, 0, 174);
        return;
    }

    // Get note name and cents deviation
    double cents_offset = 0.0;
    const char *note = frequency_to_note_name(freq, &cents_offset); //using donovan function

    lv_label_set_text(tuner_note_label, note);

    int cents = (int)round(cents_offset);

    // Frequency display
    uint32_t hz100 = (uint32_t)(freq * 100.0 + 0.5);
    lv_label_set_text_fmt(tuner_freq_label, "%lu.%02lu Hz",
                          (unsigned long)(hz100 / 100),
                          (unsigned long)(hz100 % 100));

    // Update tuning status
    if (cents >= -1 && cents <= 1)
    {
        lv_label_set_text(tuner_status_label, "IN TUNE");
        lv_obj_set_style_text_color(tuner_status_label, lv_color_hex(0x4ADE80), 0);
        lv_obj_set_style_bg_color(tuner_needle, lv_color_hex(0x4ADE80), 0);
    }
    else if (cents < 0)
    {
        lv_label_set_text_fmt(tuner_status_label, "%d cents - Flat", -cents);
        lv_obj_set_style_text_color(tuner_status_label, lv_color_hex(0xC084FC), 0);
        lv_obj_set_style_bg_color(tuner_needle, lv_color_hex(0xC084FC), 0);
    }
    else
    {
        lv_label_set_text_fmt(tuner_status_label, "%d cents - Sharp", cents);
        lv_obj_set_style_text_color(tuner_status_label, lv_color_hex(0xC084FC), 0);
        lv_obj_set_style_bg_color(tuner_needle, lv_color_hex(0xC084FC), 0);
    }

    // Needle position: -50 to +50 cents maps to -90 to +90 pixels
    if (cents < -50) cents = -50;
    if (cents > 50) cents = 50;

    int needle_x = cents * 90 / 50;
    lv_obj_align(tuner_needle, LV_ALIGN_TOP_MID, needle_x, 174);
}
