#include "display.h"
#include <atomic>

#pragma once

class Button {
    std::atomic_bool disabled;

public:
    lv_obj_t *btn = nullptr;
    lv_obj_t *lbl = nullptr;
    Button() = default;
    Button(lv_obj_t *parent) : btn(lv_btn_create(parent)), lbl(lv_label_create(btn)) {}
    Button *operator=(const Button &btn) {
        this->btn = btn.btn;
        this->lbl = btn.lbl;
        this->disabled = btn.disabled.load();
        return this;
    }
    void setLabel(const char *text) {
        lv_label_set_text(lbl, text);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    }
    void setFont(const lv_font_t *font) {
        lv_obj_set_style_text_font(lbl, font, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    }
    void setColor(lv_palette_t color) {
        lv_obj_set_style_bg_color(btn, lv_palette_main(color), 0);
        lv_obj_set_style_shadow_color(btn, lv_palette_darken(color, 2), 0);
    }
    void setOnClick(lv_event_cb_t event_cb, void *user_data = NULL) {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, user_data);
    }
    void setOnPressing(lv_event_cb_t event_cb, void *user_data = NULL) {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_PRESSING, user_data);
    }
    void setOnDown(lv_event_cb_t event_cb, void *user_data = NULL) {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_PRESSED, user_data);
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_RELEASED, user_data);
    }
    void disable(bool disable) {
        disabled = disable;
        if (disabled) {
            lv_obj_add_state(btn, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(btn, LV_STATE_DISABLED);
        }
    }
    void setAlign(lv_align_t align, lv_coord_t x_ofs = 0, lv_coord_t y_ofs = 0) {
        lv_obj_align(btn, align, x_ofs, y_ofs);
    }
    void setPos(lv_coord_t x = 0, lv_coord_t y = 0) {
        lv_obj_set_pos(btn, x, y);
    }
    void setSize(lv_coord_t w, lv_coord_t h) {
        lv_obj_set_size(btn, w, h);
    }
};