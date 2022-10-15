#include "lvgl.h"

#include "animation.h"
#include "button.h"

#pragma once

class notice;

using notice_cb_t = void (*)(notice *);

class notice {
    lv_anim_t jiggle_anim;
    lv_anim_t peek_anim;
    lv_coord_t l_moved = 0;
    Button btn;
    notice_cb_t cb = 0;

    static void click_handle(lv_event_t *e) {
        notice *self = (notice *)lv_event_get_user_data(e);
        if (self->cb)
            self->cb(self);
    }

public:
    notice(lv_obj_t *parent, notice_cb_t cb = 0, lv_coord_t pad = 8) : btn(parent), cb(cb) {
        lv_anim_init(&jiggle_anim);
        lv_anim_set_var(&jiggle_anim, btn.btn);
        lv_anim_set_exec_cb(&jiggle_anim, lv_anim_x_cb);
        lv_anim_set_path_cb(&jiggle_anim, lv_anim_pop_jiggle);

        lv_anim_init(&peek_anim);
        lv_anim_set_var(&peek_anim, btn.btn);
        lv_anim_set_exec_cb(&peek_anim, lv_anim_x_cb);
        lv_anim_set_path_cb(&peek_anim, lv_anim_path_linear);

        lv_obj_add_event_cb(btn.btn, click_handle, LV_EVENT_CLICKED, (void *)this);

        lv_obj_set_style_pad_all(btn.btn, pad, 0);
        lv_obj_set_style_pad_right(btn.btn, pad / 2, 0);
        lv_obj_set_style_text_font(btn.btn, &lv_font_montserrat_12, 0);
    }

    void setVisible(bool visible) {
        if (visible) {
            lv_obj_clear_flag(btn.btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(btn.btn, LV_OBJ_FLAG_HIDDEN);
        }
    }

    void setLabel(const char *text) {
        btn.setLabel(text);
    }

    void setHeight(lv_coord_t y, lv_coord_t pad = 4) {
        btn.setPos(-pad * 8, y);
        lv_obj_set_style_pad_left(btn.btn, pad + (pad * 8), 0);
    }

    void setColor(lv_palette_t color) {
        btn.setColor(color);
    }

    // FIXME: spamming can cause obj to move
    void jiggle(uint32_t duration = 500, int32_t amount = 25) {
        lv_anim_set_time(&jiggle_anim, duration);
        lv_anim_set_values(&jiggle_anim, lv_obj_get_x(btn.btn), lv_obj_get_x(btn.btn) + amount);
        lv_anim_start(&jiggle_anim);
    }

    void peek(uint32_t duration = 500, int32_t amount = 25) {
        lv_anim_set_time(&peek_anim, duration);
        lv_anim_set_values(&peek_anim, lv_obj_get_x(btn.btn), lv_obj_get_x(btn.btn) + amount);
        lv_anim_start(&peek_anim);
    }
};