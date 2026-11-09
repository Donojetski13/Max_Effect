/*
 * ui.c
 *
 *  Created on: Oct 12, 2026, migrated from single file
 *      Author: ewan
 */

#include "ui.h"
#include "ui_effects.h"
#include "ui_tuner.h"

// Current active screen
static Screen_t active_screen = SCREEN_MAIN;

//Screens
static lv_obj_t *main_screen = NULL;

//static lv_obj_t *effects_screen = NULL; defined in own header/implementation
static lv_obj_t *loop_screen = NULL;
static lv_obj_t *presets_screen = NULL;


// Main menu objects
static lv_obj_t *menu_items[4];
static lv_group_t *menu_group = NULL;

// Menu names
static const char *menu_names[] = {
    "Effects",
    "Looper",
    "Presets",
    "Tuner"
};

void UI_Init(void)
{
    UI_MainMenu_Init();
    UI_Effects_Init();
    UI_LoopMenu_Init();
    UI_PresetsMenu_Init();
    UI_Tuner_Init();

    lv_screen_load(main_screen);
    active_screen = SCREEN_MAIN;
}

void UI_MainMenu_Init(void)
{
    main_screen = lv_screen_active();

    lv_obj_t *screen = main_screen;

    // Clear previous screen contents
    lv_obj_clean(screen);

    // Delete previous menu group if one exists
    if(menu_group != NULL) {
        lv_group_delete(menu_group);
        menu_group = NULL;
    }

    // Background
    lv_obj_set_style_bg_color(
        screen,
        lv_color_hex(0x151D29),
        0
    );

    // Title
    lv_obj_t *title = lv_label_create(screen);

    lv_label_set_text(title, "Max Effect");

    lv_obj_set_style_text_color(
        title,
        lv_color_hex(0xFFFFFF),
        0
    );

    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    // Create LVGL menu group
    menu_group = lv_group_create();
    lv_group_set_wrap(menu_group, false);

    // Create four menu buttons
    for(int i = 0; i < 4; i++)
    {
        menu_items[i] = lv_button_create(screen);

        lv_obj_set_size(menu_items[i], 200, 45);

        lv_obj_align(
            menu_items[i],
            LV_ALIGN_TOP_MID,
            0,
            55 + i * 52
        );

        // Default color
        lv_obj_set_style_bg_color(menu_items[i], lv_color_hex(0x8582AB), LV_STATE_DEFAULT);

        // Focused color
        lv_obj_set_style_bg_color(menu_items[i], lv_color_hex(0xA858C4), LV_STATE_FOCUSED);

        // Pressed color
        lv_obj_set_style_bg_color(menu_items[i], lv_color_hex(0x9933BD), LV_STATE_PRESSED);

        // Button label
        lv_obj_t *label = lv_label_create(menu_items[i]);

        //label is lv_obj_t *label = lv_obj_get_child(menu_items[i], 0);
        lv_label_set_text(label, menu_names[i]);

        lv_obj_center(label);

        // Add button to navigation group
        lv_group_add_obj(menu_group, menu_items[i]);
    }

    // Focus first menu option
    lv_group_focus_obj(menu_items[MENU_EFFECTS]);
}

void UI_LoopMenu_Init(void){
	loop_screen = lv_obj_create(NULL);
	lv_obj_set_style_bg_color(loop_screen, lv_color_hex(0x151D29), 0);
	lv_obj_t *title = lv_label_create(loop_screen);
	lv_label_set_text(title, "Looper");
	lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
	lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);
}

void UI_PresetsMenu_Init(void){
	presets_screen = lv_obj_create(NULL);
	lv_obj_set_style_bg_color(presets_screen, lv_color_hex(0x151D29), 0);
	lv_obj_t *title = lv_label_create(presets_screen);
	lv_label_set_text(title, "Presets");
	lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
	lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);
}


void UI_Menu_Scroll(MenuDirection_t direction)
{
    if (active_screen == SCREEN_MAIN)
    {
        if (menu_group == NULL)
            return;

        if (direction == MENU_DOWN)
            lv_group_focus_next(menu_group);
        else if (direction == MENU_UP)
            lv_group_focus_prev(menu_group);
    }
    else if (active_screen == SCREEN_EFFECTS)
    {
        UI_Effects_Scroll(direction);
    }
    else if (active_screen == SCREEN_DISTORTION)
    {
        UI_Distortion_Scroll(direction);
    }
    else if(active_screen == SCREEN_WAH){
    	UI_Wah_Scroll(direction);
    }
}

void UI_SetScreen(Screen_t screen){
	switch(screen){
		case SCREEN_MAIN:
			lv_screen_load(main_screen);
			active_screen = SCREEN_MAIN;
			break;

		case SCREEN_EFFECTS:
			lv_screen_load(UI_Effects_GetScreen());
			active_screen = SCREEN_EFFECTS;
			break;
		case SCREEN_DISTORTION:
			lv_screen_load(UI_Distortion_GetScreen());
			active_screen = SCREEN_DISTORTION;
			break;
		case SCREEN_LOOPER:
			lv_screen_load(loop_screen);
			active_screen = SCREEN_LOOPER;
			break;
		case SCREEN_PRESETS:
			lv_screen_load(presets_screen);
			active_screen = SCREEN_PRESETS;
			break;
		case SCREEN_TUNER:
			lv_screen_load(UI_Tuner_GetScreen());
			active_screen = SCREEN_TUNER;
			break;
		case SCREEN_WAH:
			lv_screen_load(UI_Wah_GetScreen());
			active_screen = SCREEN_WAH;
			break;
		default:
			break;
	}
}


void UI_Menu_Enter(void)
{
    switch (active_screen)
    {
        case SCREEN_MAIN:
        {
            lv_obj_t *selected =
                lv_group_get_focused(menu_group);

            if (selected == menu_items[MENU_EFFECTS])
                UI_SetScreen(SCREEN_EFFECTS);
            else if (selected == menu_items[MENU_LOOPS])
                UI_SetScreen(SCREEN_LOOPER);
            else if (selected == menu_items[MENU_PRESETS])
                UI_SetScreen(SCREEN_PRESETS);
            else if (selected == menu_items[MENU_TUNER])
                UI_SetScreen(SCREEN_TUNER);

            break;
        }

        case SCREEN_EFFECTS:
        {
            EffectMenu_t selected = UI_Effects_GetSelection();

            if (selected == EFFECT_MENU_DISTORTION)
                UI_SetScreen(SCREEN_DISTORTION);
            else if(selected == EFFECT_MENU_WAH)
            	UI_SetScreen(SCREEN_WAH);
            else if (selected == EFFECT_MENU_BACK)
                UI_SetScreen(SCREEN_MAIN);

            break;
        }

        case SCREEN_DISTORTION:
            if (UI_Distortion_Enter())
            {
                UI_SetScreen(SCREEN_EFFECTS);
            }
            break;

        case SCREEN_WAH:
        	if(UI_Wah_Enter()){
        		UI_SetScreen(SCREEN_EFFECTS);
        	}
        	break;

        case SCREEN_LOOPER:
        case SCREEN_PRESETS:
        case SCREEN_TUNER:
            UI_SetScreen(SCREEN_MAIN);
            break;

        default:
            break;
    }
}



