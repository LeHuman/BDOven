#include "lvgl.h"
#include <algorithm>
#include <set>
#include <vector>

#include "animation.h"
#include "button.h"
#include "display.h"

#pragma once

#define JIGGLE_TIME_URGENT 5000
#define JIGGLE_TIME_DEFAULT 15000

class Notice;

using notice_cb_t = void (*)(Notice *);

#define LV_NOTICE_PAD_DEFAULT 8
const int NoticeHeight = LV_NOTICE_PAD_DEFAULT * 4;

class Notice { // TODO: animation lock
    lv_anim_t peek_anim, jiggle_anim, fade_anim, vert_anim;
    lv_coord_t l_moved = 0;
    Button btn;
    notice_cb_t cb = 0;

    static void fade_anim_cb(void *obj, int32_t v) {
        lv_obj_set_style_opa((_lv_obj_t *)obj, v, 0);
    }

    static void click_handle(lv_event_t *e) {
        Notice *self = (Notice *)lv_event_get_user_data(e);
        if (self->cb)
            self->cb(self);
    }

    static void anim_ready_handle(struct _lv_anim_t *a) {
        Notice *self = (Notice *)lv_anim_get_user_data(a);
        lv_anim_set_values(&self->fade_anim, lv_obj_get_style_opa(self->btn.btn, 0), LV_OPA_60);
        lv_anim_start(&self->fade_anim);
    }

public:
    bool urgent = false;
    int priority = 0;
    elapsedMillis ems;

    Notice(lv_obj_t *parent, notice_cb_t cb = 0, lv_coord_t pad = LV_NOTICE_PAD_DEFAULT) : btn(parent), cb(cb) {
        lv_anim_init(&jiggle_anim);
        lv_anim_set_var(&jiggle_anim, btn.btn);
        lv_anim_set_exec_cb(&jiggle_anim, lv_anim_x_cb);
        lv_anim_set_path_cb(&jiggle_anim, lv_anim_pop_jiggle);
        lv_anim_set_user_data(&jiggle_anim, this);
        lv_anim_set_ready_cb(&jiggle_anim, anim_ready_handle);

        lv_anim_init(&peek_anim);
        lv_anim_set_var(&peek_anim, btn.btn);
        lv_anim_set_exec_cb(&peek_anim, lv_anim_x_cb);
        lv_anim_set_path_cb(&peek_anim, lv_anim_path_linear);

        lv_anim_init(&vert_anim);
        lv_anim_set_time(&vert_anim, 50);
        lv_anim_set_var(&vert_anim, btn.btn);
        lv_anim_set_exec_cb(&vert_anim, lv_anim_y_cb);
        lv_anim_set_path_cb(&vert_anim, lv_anim_path_ease_in);

        lv_anim_init(&fade_anim);
        lv_anim_set_var(&fade_anim, btn.btn);
        lv_anim_set_exec_cb(&fade_anim, fade_anim_cb);

        lv_obj_add_event_cb(btn.btn, click_handle, LV_EVENT_CLICKED, (void *)this);

        lv_obj_set_style_pad_all(btn.btn, pad, 0);
        lv_obj_set_style_pad_right(btn.btn, pad / 2, 0);
        lv_obj_set_style_text_font(btn.btn, &lv_font_montserrat_12, 0);
        lv_obj_set_style_opa(btn.btn, LV_OPA_60, 0);

        lv_obj_set_x(btn.btn, (pad / 2) * -8);
        lv_obj_set_style_pad_left(btn.btn, (pad / 2) + ((pad / 2) * 8), 0);
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

    void setHeight(lv_coord_t y) {
        lv_anim_set_values(&vert_anim, lv_obj_get_y(btn.btn), y);
        lv_anim_start(&vert_anim);
    }

    void setColor(lv_palette_t color) {
        btn.setColor(color);
    }

    // FIXME: spamming can cause obj to move
    void jiggle(uint32_t duration = 500, int32_t amount = 25) {
        lv_anim_set_values(&fade_anim, lv_obj_get_style_opa(btn.btn, 0), LV_OPA_COVER);
        lv_anim_set_time(&fade_anim, duration / 4);
        lv_anim_start(&fade_anim);

        lv_anim_set_time(&jiggle_anim, duration);
        lv_anim_set_values(&jiggle_anim, lv_obj_get_x(btn.btn), lv_obj_get_x(btn.btn) + amount);
        lv_anim_start(&jiggle_anim);
    }

    // void peek(uint32_t duration = 500, int32_t amount = 25) {
    //     lv_anim_set_time(&peek_anim, duration);
    //     lv_anim_set_values(&peek_anim, lv_obj_get_x(btn.btn), lv_obj_get_x(btn.btn) + amount);
    //     lv_anim_start(&peek_anim);
    // }
};

class NoticeBoard {
    lv_obj_t *parent;
    const int y_min, y_max;
    std::vector<Notice *> notices;
    std::set<Notice *> board;
    bool invalid = false;

public:
    NoticeBoard(lv_obj_t *parent, lv_coord_t y_min = 0, lv_coord_t y_max = Display::Height) : parent(parent), y_min(y_min), y_max(y_max) {
    }
    Notice *addNotice(const char *text, notice_cb_t callback, lv_palette_t color) {
        Notice *a = new Notice(parent, callback);
        notices.push_back(a);
        a->setLabel(text);
        a->setColor(color);
        a->setVisible(false);
        return a;
    }

    void pushNotice(Notice *notice, bool urgent = false) {
        if (board.contains(notice))
            return;
        board.insert(notice);
        invalid = true;
        notice->urgent = urgent;
        notice->ems = urgent ? JIGGLE_TIME_URGENT * 3 / 4 : JIGGLE_TIME_DEFAULT * 3 / 4;
    }
    void rotateNotices() {
        std::rotate(notices.begin(), notices.begin() + 1, notices.end());
    }
    void clearNotice() {
        clearNotice(notices.back());
    }
    void clearNotice(Notice *notice) {
        board.extract(notice);
        notice->setVisible(false);
        invalid = true;
    }
    void update() {
        if (invalid) {
            int sz = board.size();
            int range = y_max - y_min;
            int step = NoticeHeight * sz > range ? range / sz : NoticeHeight + LV_NOTICE_PAD_DEFAULT / 4;
            int i = 0;
            for (auto n : board) {
                n->setHeight(y_min + (step * i++));
                n->setVisible(true);
            }
            invalid = false;
        }

        for (auto n : board) {
            if (n->urgent && n->ems > JIGGLE_TIME_URGENT) {
                n->ems = 0;
                n->jiggle(500, 50);
            } else if (n->ems > JIGGLE_TIME_DEFAULT) {
                n->ems = 0;
                n->jiggle();
            }
        }
    }
};