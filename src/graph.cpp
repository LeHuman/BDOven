#include "graph.h"
#include "spline.h"

namespace Graph {

// void event_cb(Graph *self, lv_event_t *e) {
//     static int32_t last_id = -1;
//     lv_event_code_t code = lv_event_get_code(e);
//     lv_obj_t *obj = lv_event_get_target(e);

//     if (code == LV_EVENT_VALUE_CHANGED) {
//         last_id = lv_chart_get_pressed_point(obj);
//         if (last_id != LV_CHART_POINT_NONE) {
//             lv_chart_set_cursor_point(obj, self->cursor, NULL, last_id);
//         }
//     } else if (code == LV_EVENT_DRAW_PART_END) {
//         lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
//         if (!lv_obj_draw_part_check_type(dsc, &lv_chart_class, LV_CHART_DRAW_PART_CURSOR))
//             return;
//         if (dsc->p1 == NULL || dsc->p2 == NULL || dsc->p1->y != dsc->p2->y || last_id < 0)
//             return;

//         lv_coord_t *data_array = lv_chart_get_y_array(self->chart, self->main_series);
//         lv_coord_t v = data_array[last_id];
//         char buf[16];
//         lv_snprintf(buf, sizeof(buf), "%d", v);

//         lv_point_t size;
//         lv_txt_get_size(&size, buf, LV_FONT_DEFAULT, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);

//         lv_area_t a;
//         a.y2 = dsc->p1->y - 5;
//         a.y1 = a.y2 - size.y - 10;
//         a.x1 = dsc->p1->x + 10;
//         a.x2 = a.x1 + size.x + 10;

//         lv_draw_rect_dsc_t draw_rect_dsc;
//         lv_draw_rect_dsc_init(&draw_rect_dsc);
//         draw_rect_dsc.bg_color = lv_palette_main(LV_PALETTE_BLUE);
//         draw_rect_dsc.radius = 3;

//         lv_draw_rect(dsc->draw_ctx, &draw_rect_dsc, &a);

//         lv_draw_label_dsc_t draw_label_dsc;
//         lv_draw_label_dsc_init(&draw_label_dsc);
//         draw_label_dsc.color = lv_color_white();
//         a.x1 += 5;
//         a.x2 -= 5;
//         a.y1 += 5;
//         a.y2 -= 5;
//         lv_draw_label(dsc->draw_ctx, &draw_label_dsc, &a, buf, NULL);
//     }
// }

// TODO: Time to peak

void Graph::setSize(lv_coord_t w, lv_coord_t h) {
    lv_obj_set_size(chart, w, h);
    lv_obj_refresh_ext_draw_size(chart);
}

void Graph::setPos(lv_coord_t x, lv_coord_t y, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
    lv_obj_align(chart, align, x_ofs, y_ofs);
    lv_obj_refresh_ext_draw_size(chart);
}

void Graph::setTitle(const char *text) {
    lv_label_set_text(title, text);
    lv_obj_align_to(title, chart, LV_ALIGN_OUT_TOP_MID, 0, -5);
}

void Graph::setMainData(const lv_coord_t *Xs, const lv_coord_t *Ys, const size_t length) {
    lv_chart_remove_series(chart, main_series);
    main_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREY), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_point_count(chart, length);
    for (size_t i = 0; i < length; i++) {
        lv_chart_set_next_value2(chart, main_series, Xs[i], Ys[i]);
    }
}

void Graph::setMainData(const std::vector<double> Xs, const std::vector<double> Ys, double resolution) {
    lv_chart_remove_series(chart, main_series);
    main_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREY), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_point_count(chart, ceil(Xs.back() / resolution));

    tk::spline spln(Xs, Ys, tk::spline::cspline_hermite);

    for (double i = 0.0; i < Xs.back(); i += resolution) {
        lv_chart_set_next_value2(chart, main_series, Xs[i], Ys[i]);
    }
}

void Graph::init(void) {
    lv_chart_set_type(chart, LV_CHART_TYPE_SCATTER);
    lv_obj_set_size(chart, 230, 150);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, -10);

    // lv_obj_add_event_cb(chart, event_cb, LV_EVENT_ALL, NULL);
    main_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREY), LV_CHART_AXIS_PRIMARY_Y);
    alt_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_CYAN), LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);

    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 10, 5, 6, 5, true, 40);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 10, 5, 10, 1, true, 30);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, 300);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 300);

    cursor = lv_chart_add_cursor(chart, lv_palette_main(LV_PALETTE_BLUE_GREY), LV_DIR_LEFT | LV_DIR_BOTTOM);
}

} // namespace Graph