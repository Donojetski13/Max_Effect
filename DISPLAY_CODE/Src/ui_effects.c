/*
 * ui_effects.c
 *
 *  Created on: Oct 12, 2026
 *      Author: ewan
 */
#include "ui_effects.h"
#include "pedal_type.h"

//screens owned by this module (effects sub)
static lv_obj_t *effects_screen = NULL;
static lv_obj_t *distortion_screen = NULL;
static lv_obj_t *wah_screen = NULL;
//other screens tbh

//Menu object
static lv_obj_t *effect_items[6];
static lv_group_t *effects_group = NULL;

static const char *effect_names[] = {
		"Distortion",
		"Reverb",
		"Delay",
		"Pitch",
		"Wah",
		"Back"
};

#define DIST_PARAM_COUNT 3
#define DIST_BACK_INDEX 3
static int8_t dist_selected = 0;
static bool dist_editing = false;
static lv_obj_t *dist_back_label = NULL;
static void UI_Distortion_UpdateFocus(void);
static void UI_Wah_Init(void);


static void UI_EffectsMenu_Init(void){
    effects_screen = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(
        effects_screen, lv_color_hex(0x151D29), 0);

    lv_obj_set_scrollable(effects_screen, false);

    lv_obj_t *title = lv_label_create(effects_screen);
    lv_label_set_text(title, "Effects");
    lv_obj_set_style_text_color(
        title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    effects_group = lv_group_create();
    lv_group_set_wrap(effects_group, false);

    for (int i = 0; i < 6; i++)
    {
        effect_items[i] = lv_button_create(effects_screen);

        lv_obj_set_size(effect_items[i], 200, 34);
        lv_obj_align(
            effect_items[i], LV_ALIGN_TOP_MID,
            0, 40 + i * 38);

        lv_obj_set_style_bg_color(
            effect_items[i], lv_color_hex(0x8582AB),
            LV_STATE_DEFAULT);

        lv_obj_set_style_bg_color(
            effect_items[i], lv_color_hex(0xA858C4),
            LV_STATE_FOCUSED);

        lv_obj_t *label = lv_label_create(effect_items[i]);
        lv_label_set_text(label, effect_names[i]);
        lv_obj_center(label);

        lv_group_add_obj(effects_group, effect_items[i]);
    }

    lv_group_focus_obj(effect_items[EFFECT_MENU_DISTORTION]);
}


//distortion parameter data
static const char *dist_names[] = {
		"Gain", "Tone", "Level"
};
static int32_t dist_values[DIST_PARAM_COUNT] = {70, 50, 85};
static lv_obj_t *dist_arcs[DIST_PARAM_COUNT];
static lv_obj_t *dist_value_labels[DIST_PARAM_COUNT];
static lv_obj_t *dist_containers[DIST_PARAM_COUNT];



static void UI_Distortion_Init(void){
    distortion_screen = lv_obj_create(NULL);

    // Background
    lv_obj_set_style_bg_color(distortion_screen, lv_color_hex(0x151D29), 0);
    lv_obj_set_scrollable(distortion_screen, false);

    // Title
    lv_obj_t *title = lv_label_create(distortion_screen);
    lv_label_set_text(title, "Distortion");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Parameter rows
    for(int i = 0; i < DIST_PARAM_COUNT; i++)
    {
        int y = 30 + i * 72;

        // Container for arc + name + value
        dist_containers[i] = lv_obj_create(distortion_screen);
        lv_obj_set_size(dist_containers[i], 220, 72);
        lv_obj_align(dist_containers[i], LV_ALIGN_TOP_MID, 0, y);
        lv_obj_set_style_bg_opa(dist_containers[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(dist_containers[i], 0, 0);
        lv_obj_set_style_pad_all(dist_containers[i], 0, 0);
        lv_obj_set_scrollable(dist_containers[i], false);
        lv_obj_set_clickable(dist_containers[i], false);

        // Arc
        dist_arcs[i] = lv_arc_create(dist_containers[i]);
        lv_obj_set_size(dist_arcs[i], 65, 65);
        lv_obj_align(dist_arcs[i], LV_ALIGN_LEFT_MID, 8, 0);
        lv_arc_set_range(dist_arcs[i], 0, 100);
        lv_arc_set_rotation(dist_arcs[i], 135);
        lv_arc_set_bg_angles(dist_arcs[i], 0, 270);
        lv_arc_set_value(dist_arcs[i], dist_values[i]);

        // Arc styling
        lv_obj_set_style_arc_color(dist_arcs[i], lv_color_hex(0x384153), LV_PART_MAIN);
        lv_obj_set_style_arc_color(dist_arcs[i], lv_color_hex(0xA858C4), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(dist_arcs[i], lv_color_hex(0xFFFFFF), LV_PART_KNOB);
        lv_obj_set_style_pad_all(dist_arcs[i], 2, LV_PART_KNOB);
        lv_obj_set_clickable(dist_arcs[i], false);

        // Parameter name
        lv_obj_t *name = lv_label_create(dist_containers[i]);
        lv_label_set_text(name, dist_names[i]);
        lv_obj_set_style_text_color(name, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 95, -12);

        // Numeric value
        dist_value_labels[i] = lv_label_create(dist_containers[i]);
        lv_label_set_text_fmt(dist_value_labels[i], "%ld", (long)dist_values[i]);
        lv_obj_set_style_text_color(dist_value_labels[i], lv_color_hex(0x8582AB), 0);
        lv_obj_align(dist_value_labels[i], LV_ALIGN_LEFT_MID, 95, 12);
    }

    // Back option
    dist_back_label = lv_label_create(distortion_screen);
    lv_label_set_text(dist_back_label, "Back");
    lv_obj_align(dist_back_label, LV_ALIGN_BOTTOM_MID, 0, -8);

    UI_Distortion_UpdateFocus();
}


static void UI_Distortion_UpdateFocus(void)
{
    for(int i = 0; i < DIST_PARAM_COUNT; i++)
    {
        bool selected = (dist_selected == i);

        // Highlight entire parameter row
        lv_obj_set_style_border_width(dist_containers[i], selected ? 2 : 0, 0);
        lv_obj_set_style_border_color(dist_containers[i], dist_editing && selected ? lv_color_hex(0xC084FC) : lv_color_hex(0xA858C4), 0);
        lv_obj_set_style_radius(dist_containers[i], 8, 0);

        // Highlight arc indicator when editing
        lv_obj_set_style_arc_color(dist_arcs[i], dist_editing && selected ? lv_color_hex(0xC084FC) : lv_color_hex(0xA858C4), LV_PART_INDICATOR);
    }

    // Highlight Back option
    lv_obj_set_style_text_color(dist_back_label, dist_selected == DIST_BACK_INDEX ? lv_color_hex(0xA858C4) : lv_color_hex(0x8582AB), 0);
}


void UI_Distortion_Scroll(MenuDirection_t direction)
{
    if (dist_editing)
    {
        int32_t value = dist_values[dist_selected];

        if (direction == MENU_DOWN)
            value++;
        else if (direction == MENU_UP)
            value--;

        // Clamp 0-100
        if (value < 0) value = 0;
        if (value > 100) value = 100;

        dist_values[dist_selected] = value;

        lv_arc_set_value(
            dist_arcs[dist_selected], value);

        lv_label_set_text_fmt(
            dist_value_labels[dist_selected],
            "%ld", (long)value);
    }
    else
    {
        // Navigate through Gain, Tone, Level, Back
        if (direction == MENU_DOWN &&
            dist_selected < DIST_BACK_INDEX)
        {
            dist_selected++;
        }
        else if (direction == MENU_UP &&
                 dist_selected > 0)
        {
            dist_selected--;
        }

        UI_Distortion_UpdateFocus();
    }
}

bool UI_Distortion_Enter(void)
{
    if (dist_editing)
    {
        // Exit editing mode
        dist_editing = false;
        UI_Distortion_UpdateFocus();
        return false;
    }

    if (dist_selected == DIST_BACK_INDEX)
    {
        return true;  // Request return to Effects menu
    }

    // Enter editing mode
    dist_editing = true;
    UI_Distortion_UpdateFocus();

    return false;
}

void UI_Effects_Init(void){
	UI_EffectsMenu_Init();
	UI_Distortion_Init();
	UI_Wah_Init();
}

lv_obj_t *UI_Effects_GetScreen(void)
{
    return effects_screen;
}

void UI_Effects_Scroll(MenuDirection_t direction)
{
    if (effects_group == NULL)
        return;

    if (direction == MENU_DOWN)
        lv_group_focus_next(effects_group);
    else if (direction == MENU_UP)
        lv_group_focus_prev(effects_group);
}

EffectMenu_t UI_Effects_GetSelection(void)
{
    if (effects_group == NULL)
        return EFFECT_MENU_BACK;

    lv_obj_t *selected = lv_group_get_focused(effects_group);

    for (int i = 0; i < 6; i++)
    {
        if (selected == effect_items[i])
            return (EffectMenu_t)i;
    }

    return EFFECT_MENU_BACK;
}

lv_obj_t *UI_Distortion_GetScreen(void){
	return distortion_screen;
}


// Wah screen
#define WAH_FREQ_INDEX 0
#define WAH_TEST_INDEX 1
#define WAH_BACK_INDEX 2

static lv_obj_t *wah_slider = NULL;
static lv_obj_t *wah_hz_label = NULL;
static lv_obj_t *wah_value_label = NULL;
static lv_obj_t *wah_test_button = NULL;
static lv_obj_t *wah_status_label = NULL;
static lv_obj_t *wah_back_label = NULL;

static int8_t wah_selected = WAH_FREQ_INDEX;
static bool wah_editing = false;

// Shared Wah DSP parameter
static float wah_settings[1] = {50.0f};

static params wah_params = {
    .enable = true,
    .settings = wah_settings
};

#define WAH_MIN_HZ 40
#define WAH_MAX_HZ 2000

static int UI_Wah_GetFrequency(void)
{
    return WAH_MIN_HZ + (int)(wah_settings[0] * (WAH_MAX_HZ - WAH_MIN_HZ) / 100.0f);
}

static void UI_Wah_UpdateDisplay(void)
{
    int value = (int)wah_settings[0];

    lv_slider_set_value(wah_slider, value, LV_ANIM_OFF);
    lv_label_set_text_fmt(wah_value_label, "%d%%", value);
    lv_label_set_text_fmt(wah_hz_label, "%d Hz", UI_Wah_GetFrequency());
}

static void UI_Wah_UpdateFocus(void)
{
    bool freq_selected = (wah_selected == WAH_FREQ_INDEX);
    bool test_selected = (wah_selected == WAH_TEST_INDEX);
    bool back_selected = (wah_selected == WAH_BACK_INDEX);

    lv_obj_set_style_outline_width(wah_slider, freq_selected ? 2 : 0, 0);
    lv_obj_set_style_outline_color(wah_slider, wah_editing ? lv_color_hex(0x4ADE80) : lv_color_hex(0xA858C4), 0);
    lv_obj_set_style_outline_pad(wah_slider, 4, 0);

    lv_obj_set_style_bg_color(wah_test_button, test_selected ? lv_color_hex(0xA858C4) : lv_color_hex(0x8582AB), 0);

    lv_obj_set_style_text_color(wah_back_label, back_selected ? lv_color_hex(0xA858C4) : lv_color_hex(0x8582AB), 0);
}


static void UI_Wah_Init(void)
{
    wah_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(wah_screen, lv_color_hex(0x151D29), 0);
    lv_obj_set_scrollable(wah_screen, false);

    // Title
    lv_obj_t *title = lv_label_create(wah_screen);
    lv_label_set_text(title, "Wah");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 18, 12);

    // ON indicator
    lv_obj_t *enabled_label = lv_label_create(wah_screen);
    lv_label_set_text(enabled_label, "ON");
    lv_obj_set_style_text_color(enabled_label, lv_color_hex(0x4ADE80), 0);
    lv_obj_align(enabled_label, LV_ALIGN_TOP_RIGHT, -20, 12);

    // Percentage
    wah_value_label = lv_label_create(wah_screen);
    lv_label_set_text(wah_value_label, "50%");
    lv_obj_set_style_text_color(wah_value_label, lv_color_hex(0x4ADE80), 0);
#if LV_FONT_MONTSERRAT_32
    lv_obj_set_style_text_font(wah_value_label, &lv_font_montserrat_32, 0);
#endif
    lv_obj_align(wah_value_label, LV_ALIGN_TOP_MID, 0, 55);

    // Frequency in Hz
    wah_hz_label = lv_label_create(wah_screen);
    lv_label_set_text(wah_hz_label, "949 Hz");
    lv_obj_set_style_text_color(wah_hz_label, lv_color_hex(0xA3A3B6), 0);
    lv_obj_align(wah_hz_label, LV_ALIGN_TOP_MID, 0, 100);

    // Horizontal Wah slider
    wah_slider = lv_slider_create(wah_screen);
    lv_obj_set_size(wah_slider, 190, 12);
    lv_obj_align(wah_slider, LV_ALIGN_TOP_MID, 0, 145);
    lv_slider_set_range(wah_slider, 0, 100);
    lv_slider_set_value(wah_slider, 50, LV_ANIM_OFF);

    // Gray track
    lv_obj_set_style_bg_color(wah_slider, lv_color_hex(0xB0B5BF), LV_PART_MAIN);
    lv_obj_set_style_radius(wah_slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);

    // Hide filled section
    lv_obj_set_style_bg_opa(wah_slider, LV_OPA_TRANSP, LV_PART_INDICATOR);

    // White slider knob
    lv_obj_set_style_bg_color(wah_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_pad_all(wah_slider, 5, LV_PART_KNOB);

    // Encoder controls the slider
    lv_obj_set_clickable(wah_slider, false);

    // Bass label
    lv_obj_t *bass_label = lv_label_create(wah_screen);
    lv_label_set_text(bass_label, "Bass");
    lv_obj_set_style_text_color(bass_label, lv_color_hex(0xA3A3B6), 0);
    lv_obj_align(bass_label, LV_ALIGN_TOP_LEFT, 15, 169);

    // Treble label
    lv_obj_t *treble_label = lv_label_create(wah_screen);
    lv_label_set_text(treble_label, "Treble");
    lv_obj_set_style_text_color(treble_label, lv_color_hex(0xA3A3B6), 0);
    lv_obj_align(treble_label, LV_ALIGN_TOP_RIGHT, -15, 169);

    // Test Effect button
    wah_test_button = lv_button_create(wah_screen);
    lv_obj_set_size(wah_test_button, 190, 35);
    lv_obj_align(wah_test_button, LV_ALIGN_TOP_MID, 0, 195);
    lv_obj_set_style_bg_color(wah_test_button, lv_color_hex(0x8582AB), 0);

    lv_obj_t *test_label = lv_label_create(wah_test_button);
    lv_label_set_text(test_label, "Test Effect");
    lv_obj_center(test_label);

    // Test status
    wah_status_label = lv_label_create(wah_screen);
    lv_label_set_text(wah_status_label, "Ready to test");
    lv_obj_set_style_text_color(wah_status_label, lv_color_hex(0x8582AB), 0);
    lv_obj_align(wah_status_label, LV_ALIGN_TOP_MID, 0, 234);

    // Back
    wah_back_label = lv_label_create(wah_screen);
    lv_label_set_text(wah_back_label, "Back");
    lv_obj_align(wah_back_label, LV_ALIGN_BOTTOM_MID, 0, -8);

    UI_Wah_UpdateDisplay();
    UI_Wah_UpdateFocus();
}



void UI_Wah_Scroll(MenuDirection_t direction)
{
    if (wah_editing)
    {
        int32_t value = (int32_t)wah_settings[0];

        if (direction == MENU_DOWN)
            value++;
        else if (direction == MENU_UP)
            value--;

        // Clamp 0-100
        if (value < 0) value = 0;
        if (value > 100) value = 100;

        // Update shared DSP parameter
        wah_settings[0] = (float)value;

        // Update LVGL
        UI_Wah_UpdateDisplay();
        // Previous test no longer represents current settings
        lv_label_set_text(wah_status_label, "Ready to test");
        lv_obj_set_style_text_color(wah_status_label, lv_color_hex(0x8582AB), 0);
    }
    else
    {
        if (direction == MENU_DOWN && wah_selected < WAH_BACK_INDEX)
            wah_selected++;
        else if (direction == MENU_UP && wah_selected > 0)
            wah_selected--;

        UI_Wah_UpdateFocus();
    }
}


bool UI_Wah_Enter(void)
{
    if (wah_editing)
    {
        // Exit parameter editing
        wah_editing = false;
        UI_Wah_UpdateFocus();
        return false;
    }

    if (wah_selected == WAH_BACK_INDEX)
    {
        return true;
    }

    if (wah_selected == WAH_FREQ_INDEX)
    {
        // Enter parameter editing
        wah_editing = true;
        UI_Wah_UpdateFocus();
        return false;
    }

    if (wah_selected == WAH_TEST_INDEX)
    {
        // Read the parameter that the DSP will use
        int32_t test_value = (int32_t)wah_params.settings[0];

        // Confirm current value on TFT
        lv_label_set_text_fmt(wah_status_label, "Param received: %ld", (long)test_value);
        lv_obj_set_style_text_color(wah_status_label, lv_color_hex(0x4ADE80), 0);

        // Later: trigger a real DSP/audio test here
        return false;
    }

    return false;
}

lv_obj_t *UI_Wah_GetScreen(void)
{
    return wah_screen;
}

void UI_Wah_GetSettings(float *frequency, bool *enabled)
{
    if (frequency != NULL)
        *frequency = wah_params.settings[0];

    if (enabled != NULL)
        *enabled = wah_params.enable;
}






