
#pragma once
#include <lvgl.h>
#include <vector>

namespace Graph {

class Graph {
private:
    lv_obj_t *chart;
    lv_chart_series_t *main_series;
    lv_chart_series_t *alt_series;
    lv_chart_cursor_t *cursor;
    lv_obj_t *title;

    void init(void);
    // friend void event_cb(Graph *self, lv_event_t *e);

public:
    Graph(lv_obj_t *parent = lv_scr_act()) : chart(lv_chart_create(parent)), title(lv_label_create(parent)) { init(); };

    void setTitle(const char *text);
    void setMainData(const lv_coord_t *Xs, const lv_coord_t *Ys, const size_t length);
    void setMainData(const std::vector<double> &Xs, const std::vector<double> &Ys, double resolution = 0.5);
    void setSize(lv_coord_t w, lv_coord_t h);
    void setPos(lv_coord_t x, lv_coord_t y, lv_align_t align = LV_ALIGN_CENTER, lv_coord_t x_ofs = 0, lv_coord_t y_ofs = 0);
};

} // namespace Graph