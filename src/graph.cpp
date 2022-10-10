#include "graph.h"
#include "spline.h"
#include "util.h"

namespace Graph {

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

lv_coord_t Graph::updateData(lv_coord_t x, lv_coord_t y) {
    auto Xs = lv_chart_get_x_array(chart, main_series);
    auto Ys = lv_chart_get_y_array(chart, main_series);
    int i = 0;
    for (; i < pnt_cnt; ++i) {
        if (*(Xs + i) >= x)
            break;
    }
    if (y > y_max) {
        y_max += g_margin;
        updateRange();
    }
    lv_coord_t curr_y = *(Ys + i);
    lv_chart_set_value_by_id(chart, alt_series, i, y);

    if (y >= curr_y) {
        top_cur->color = lv_palette_main(LV_PALETTE_PINK);
        btm_cur->color = lv_palette_main(LV_PALETTE_BLUE);
        lv_chart_set_cursor_point(chart, btm_cur, main_series, i);
        lv_chart_set_cursor_point(chart, top_cur, alt_series, i);
    } else {
        top_cur->color = lv_palette_main(LV_PALETTE_BLUE);
        btm_cur->color = lv_palette_main(LV_PALETTE_PINK);
        lv_chart_set_cursor_point(chart, btm_cur, alt_series, i);
        lv_chart_set_cursor_point(chart, top_cur, main_series, i);
    }
    return curr_y;
}

void Graph::updateRange() {
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, x_max + g_margin);
    lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_X, 0, x_max + g_margin);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, y_max + g_margin);
    lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, 0, y_max + g_margin);
}

void Graph::setMainData(const std::vector<double> &Xs, const std::vector<double> &Ys, double resolution) {
    pnt_cnt = ceil(Xs.back() / resolution) + 1;
    lv_chart_set_point_count(chart, pnt_cnt);

    tk::spline spln(Xs, Ys, tk::spline::cspline_hermite);

    for (int i = 0; i < pnt_cnt; i++) {
        lv_chart_set_next_value2(chart, main_series, resolution * i, spln(resolution * i));
        lv_chart_set_next_value2(chart, alt_series, resolution * i, spln(resolution * i));
    }

    for (auto it1 = Xs.begin(), it2 = Ys.begin(); it1 != Xs.end(); ++it1, ++it2) {
        x_max = max(*it1, x_max);
        y_max = max(*it2, y_max);
    }

    lv_chart_refresh(chart);
    updateRange();
}

void Graph::zoom(uint16_t zoom) {
    zoom = cmap(zoom, 0, 100, 0, 1024);
    lv_chart_set_zoom_x(chart, zoom);
    lv_chart_set_zoom_y(chart, zoom);
}

void Graph::init(void) {
    lv_chart_set_type(chart, LV_CHART_TYPE_SCATTER);
    lv_obj_set_size(chart, 230, 150);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, -10);

    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);

    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 10, 5, 6, 5, true, 40);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 10, 5, 8, 4, true, 30);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

    main_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_INDIGO), LV_CHART_AXIS_PRIMARY_Y);
    alt_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_SECONDARY_Y);

    top_cur = lv_chart_add_cursor(chart, lv_palette_main(LV_PALETTE_RED), LV_DIR_TOP);
    btm_cur = lv_chart_add_cursor(chart, lv_palette_main(LV_PALETTE_BLUE), LV_DIR_BOTTOM);
}

} // namespace Graph