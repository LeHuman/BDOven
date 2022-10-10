#include "Arduino.h"
#include "display.h"
#include "lv_conf.h"
#include "spline.h"
#include <vector>

#include "graph.h"
#include "reflow.h"

int main(void) {
    // TODO: Time to peak
    Display::init();

    lv_obj_t *tabview = lv_tabview_create(lv_scr_act(), LV_DIR_RIGHT, 35);
    lv_obj_set_style_bg_color(tabview, lv_palette_darken(LV_PALETTE_GREY, 4), 0);

    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_style_bg_color(tab_btns, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_text_color(tab_btns, lv_palette_lighten(LV_PALETTE_GREY, 5), 0);
    lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_LEFT, LV_PART_ITEMS | LV_STATE_CHECKED);

    lv_obj_t *tab_ctrl = lv_tabview_add_tab(tabview, LV_SYMBOL_HOME);     // Controls
    lv_obj_t *tab_grph = lv_tabview_add_tab(tabview, LV_SYMBOL_IMAGE);    // Graph
    lv_obj_t *tab_selc = lv_tabview_add_tab(tabview, LV_SYMBOL_LIST);     // Paste Select
    lv_obj_t *tab_sett = lv_tabview_add_tab(tabview, LV_SYMBOL_SETTINGS); // Settings
    lv_obj_t *tab_abut = lv_tabview_add_tab(tabview, LV_SYMBOL_WARNING);  // About

    char buf[128];
    Graph::Graph g(tab_grph);
    g.setSize(Display::Width - 82, Display::Height - 54);
    g.setPos(-18, -2);

    lv_tabview_set_act(tabview, 2, LV_ANIM_ON);

    // lv_obj_move_to(tabview, 35, 0);
    // lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_OFF);
    // lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLL_ELASTIC);

    while (true) {
        for (auto &profile : Reflow::PROFILES) {
            double val = 0;
            double time = 0;
            int32_t rnd = 0;
            elapsedMillis ems, tms;
            g.setMainData(profile.Xs, profile.Ys);
            Reflow::title(profile, buf, 128);
            g.setTitle(buf);
            tk::spline spln(profile.Xs, profile.Ys, tk::spline::cspline_hermite);

            while (true) {
                if (ems > 5) {
                    ems = 0;
                    val = spln(time) + (rnd += random(-4, 5));
                    g.updateData(time, val);
                    time += 1;
                    if (time > profile.Xs.back())
                        break;
                }
                if (tms > 50) {
                    tms = 0;
                    Reflow::stateString(profile, val, time, buf, 128);
                    g.setSubText(buf);
                }
                lv_task_handler();
            }
        }
    }

    return 0;
}
