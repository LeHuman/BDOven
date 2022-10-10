#include <lvgl.h>
#include <stdint.h>
#include <vector>

#pragma once

namespace Graph {

class Graph {
public:
    const bool min_graph = false;
    const lv_coord_t g_margin = 15;
    uint16_t pnt_cnt = 0;
    lv_coord_t x_max = 0, y_max = 0;
    lv_obj_t *chart;
    lv_chart_series_t *main_series = nullptr;
    lv_chart_series_t *alt_series = nullptr;
    lv_chart_cursor_t *btm_cur = nullptr;
    lv_chart_cursor_t *top_cur = nullptr;
    lv_obj_t *sub_text;
    lv_obj_t *title;
    std::vector<lv_chart_cursor_t *> points;

    void init(void);
    void min_init(void);
    void updateRange();

    Graph(lv_obj_t *parent, bool) : min_graph(true), chart(lv_chart_create(parent)) { min_init(); };
    Graph(lv_obj_t *parent) : chart(lv_chart_create(parent)), title(lv_label_create(parent)), sub_text(lv_label_create(parent)) { init(); };

    void zoom(uint16_t zoom);
    void setVisible(bool visible);
    void setTitle(const char *text);
    void setSubText(const char *text);
    lv_coord_t updateData(lv_coord_t x, lv_coord_t y);
    void setMainData(const std::vector<double> &Xs, const std::vector<double> &Ys, double resolution = 1);
    void setSize(lv_coord_t w, lv_coord_t h);
    void align(lv_align_t align = LV_ALIGN_CENTER, lv_coord_t x_ofs = 0, lv_coord_t y_ofs = 0);
    void setPos(lv_coord_t x, lv_coord_t y);
};

} // namespace Graph