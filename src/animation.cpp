#include "animation.h"
#include "spline.h"
#include "util.h"

void lv_anim_x_cb(void *var, int32_t v) {
    lv_obj_set_x((_lv_obj_t *)var, v);
}

void lv_anim_y_cb(void *var, int32_t v) {
    lv_obj_set_y((_lv_obj_t *)var, v);
}

// TODO: Convert to lookup
tk::spline lv_anim_pop_jiggle_spln({0, 150, 400, 525, 650, 775, 900, 1000}, {0, -0.5, 0.5, 0, 0.25, 0, 0.125, 0}, tk::spline::cspline_hermite);

int32_t lv_anim_pop_jiggle(const lv_anim_t *a) {
    uint32_t t = cmap(a->act_time, 0, a->time, 0, 1000);
    int32_t diff = a->start_value - a->end_value;
    return a->start_value + lv_anim_pop_jiggle_spln(t) * diff;
}