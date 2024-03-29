#include "graph.h"
#include "spline.h"
#include "util.h"

namespace Graph {

void Graph::setSize(lv_coord_t w, lv_coord_t h) {
    lv_obj_set_size(chart, w, h);
    lv_obj_refresh_ext_draw_size(chart);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 0);
}

void Graph::align(lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
    lv_obj_align(chart, align, x_ofs, y_ofs);
    if (!min_graph) { // TODO: make class template instead of ifs everywhere
        lv_obj_align_to(title, chart, LV_ALIGN_OUT_TOP_MID, 0, -5);
        lv_obj_align_to(sub_text, chart, LV_ALIGN_OUT_BOTTOM_LEFT, 10, -22);
    }
    lv_obj_refresh_ext_draw_size(chart);
}

void Graph::setPos(lv_coord_t x, lv_coord_t y) {
    lv_obj_set_pos(chart, x, y);
    if (!min_graph) {
        lv_obj_align_to(title, chart, LV_ALIGN_OUT_TOP_MID, 0, -5);
        lv_obj_align_to(sub_text, chart, LV_ALIGN_OUT_BOTTOM_LEFT, 10, -22);
    }
    lv_obj_refresh_ext_draw_size(chart);
}

void Graph::setTitle(const char *text) {
    lv_label_set_text(title, text);
    lv_obj_align_to(title, chart, LV_ALIGN_OUT_TOP_MID, 0, -5);
}

void Graph::setSubText(const char *text) {
    lv_label_set_text(sub_text, text);
    lv_obj_align_to(sub_text, chart, LV_ALIGN_OUT_BOTTOM_LEFT, 10, -22);
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
        if (!min_graph)
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

void Graph::setVisible(bool visible) {
    if (visible) {
        lv_obj_clear_flag(chart, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(chart, LV_OBJ_FLAG_HIDDEN);
    }
}

void Graph::init(void) {
    lv_chart_set_type(chart, LV_CHART_TYPE_SCATTER);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);
    lv_obj_set_style_size(chart, 4, 4, LV_PART_CURSOR);

    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_SECONDARY_Y, 10, 5, 6, 5, true, 40);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 10, 5, 8, 4, true, 30);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

    lv_obj_set_style_line_color(chart, lv_palette_main(LV_PALETTE_GREY), LV_PART_TICKS);
    lv_obj_set_style_text_font(sub_text, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(sub_text, lv_palette_darken(LV_PALETTE_GREY, 4), 0);

    main_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_INDIGO), LV_CHART_AXIS_PRIMARY_Y);
    alt_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_SECONDARY_Y);

    top_cur = lv_chart_add_cursor(chart, lv_palette_main(LV_PALETTE_RED), LV_DIR_TOP);
    btm_cur = lv_chart_add_cursor(chart, lv_palette_main(LV_PALETTE_BLUE), LV_DIR_BOTTOM);
}

void Graph::min_init(void) {
    lv_chart_set_type(chart, LV_CHART_TYPE_SCATTER);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(chart, 0, 0);
    lv_obj_set_style_outline_pad(chart, 0, 0);
    lv_chart_set_div_line_count(chart, 0, 0);

    main_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
}

} // namespace Graph