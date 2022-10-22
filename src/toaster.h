#include "animation.h"
#include "display.h"
#include "lvgl.h"

#pragma once

class Toaster {
    const int pad = 12;
    lv_anim_t out_anim;
    lv_obj_t *base;
    lv_obj_t *lbl = nullptr;
    lv_coord_t lx = 0, ly = 0;
    lv_coord_t curr_width = 0;

    void setVisible(bool visible) {
        if (visible) {
            lv_obj_clear_flag(base, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(base, LV_OBJ_FLAG_HIDDEN);
        }
    }

    void setPos() {
        setPos(lx, ly);
    }

    static void out_ready_handle(struct _lv_anim_t *a) {
        Toaster *self = (Toaster *)lv_anim_get_user_data(a);
        self->setVisible(false);
    }

public:
    Toaster() = default;
    Toaster(lv_obj_t *parent) : base(lv_obj_create(parent)), lbl(lv_label_create(base)) {
        lv_anim_init(&out_anim);
        lv_anim_set_var(&out_anim, base);
        lv_anim_set_exec_cb(&out_anim, lv_anim_x_cb);
        lv_anim_set_user_data(&out_anim, this);
        lv_anim_set_path_cb(&out_anim, lv_anim_path_ease_out);
        lv_anim_set_ready_cb(&out_anim, out_ready_handle);
        lv_anim_set_time(&out_anim, 250);
        lv_anim_set_delay(&out_anim, 750);

        lv_obj_set_width(base, lv_obj_get_width(lbl) + pad);
        lv_obj_set_height(base, lv_obj_get_height(lbl) + pad);
        lv_obj_set_scrollbar_mode(base, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_border_opa(base, LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_opa(base, LV_OPA_70, 0);
        lv_obj_set_style_text_color(base, lv_palette_lighten(LV_PALETTE_GREY, 4), 0);
        lv_obj_add_state(base, LV_STATE_DISABLED);
        lv_obj_add_state(lbl, LV_STATE_DISABLED);
        setVisible(false);
    }
    Toaster *operator=(const Toaster &t) {
        this->lbl = t.lbl; // TODO: Free if exists
        this->base = t.base;
        this->out_anim = t.out_anim;
        this->lx = t.lx;
        this->ly = t.ly;
        return this;
    }
    void setPos(lv_coord_t x, lv_coord_t y) {
        lx = x;
        ly = y;
        lv_obj_set_pos(base, x, y);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    }
    void setFont(const lv_font_t *font) {
        lv_obj_set_style_text_font(base, font, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    }
    void setColor(lv_palette_t color) {
        lv_obj_set_style_bg_color(base, lv_palette_main(color), 0);
    }
    void toast(const char *text) {
        lv_label_set_text(lbl, text);
        auto font = lv_obj_get_style_text_font(base, 0);
        curr_width = lv_txt_get_width(text, strlen(text), font, 0, 0) + pad * 2;
        lv_obj_set_width(base, curr_width);
        lv_obj_set_height(base, font->line_height + pad * 1);

        lv_anim_set_values(&out_anim, lx, lx - curr_width - pad * 2);
        lv_anim_start(&out_anim);
        setVisible(true);
    }
};
