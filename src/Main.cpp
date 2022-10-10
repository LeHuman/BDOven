#include "Arduino.h"
#include "lv_conf.h"
#include "spline.h"
#include <vector>

#include "display.h"
#include "graph.h"
#include "reflow.h"
#include "select.h"

lv_obj_t *tabview;
Graph::Graph *main_graph;
Graph::Graph *prev_graph;
static lv_obj_t *currentButton = NULL;
const Reflow::ReflowProfile *select_profile;
const Reflow::ReflowProfile *active_profile;

static void event_handler_select(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED) {
        if (currentButton == obj) {
            currentButton = NULL;
        } else {
            currentButton = obj;
        }
        lv_obj_t *parent = lv_obj_get_parent(obj);
        for (uint32_t i = 0; i < lv_obj_get_child_cnt(parent); i++) {
            lv_obj_t *child = lv_obj_get_child(parent, i);
            if (child == currentButton) {
                lv_obj_add_state(child, LV_STATE_CHECKED);
                select_profile = (const Reflow::ReflowProfile *)lv_event_get_user_data(e);
                prev_graph->setMainData(select_profile->Xs, select_profile->Ys);
            } else {
                lv_obj_clear_state(child, LV_STATE_CHECKED);
            }
        }
    }
}

static void event_handler_confirm_select(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        active_profile = select_profile;
        lv_tabview_set_act(tabview, 0, LV_ANIM_ON);
    }
}

int main(void) {
    // TODO: Time to peak
    Display::init();

    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_RIGHT, lv_pct(10));
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
    main_graph = &g;
    g.setSize(lv_pct(90), lv_pct(85));
    g.setPos(-20, -2);

    lv_obj_t *list;

    Graph::Graph pg(tab_selc, true);
    prev_graph = &pg;
    pg.setSize(lv_pct(80), lv_pct(40));
    pg.align(LV_ALIGN_TOP_LEFT);

    lv_obj_t *btn_sel_confirm = lv_btn_create(tab_selc);
    lv_obj_set_size(btn_sel_confirm, lv_pct(20), lv_pct(35));
    lv_obj_align(btn_sel_confirm, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(btn_sel_confirm, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_shadow_color(btn_sel_confirm, lv_palette_darken(LV_PALETTE_GREEN, 2), 0);
    lv_obj_add_event_cb(btn_sel_confirm, event_handler_confirm_select, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_sel_confirm_l = lv_label_create(btn_sel_confirm);
    lv_label_set_text(btn_sel_confirm_l, LV_SYMBOL_OK);
    lv_obj_align(btn_sel_confirm_l, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(btn_sel_confirm_l, &lv_font_montserrat_24, 0);

    list = lv_list_create(tab_selc);
    lv_obj_set_size(list, lv_pct(100), lv_pct(60));
    lv_obj_set_style_pad_row(list, 5, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_style_outline_pad(list, 0, 0);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ACTIVE);

    lv_obj_t *btn;
    int i;
    for (auto &profile : Reflow::PROFILES) {
        btn = lv_btn_create(list);
        lv_obj_set_width(btn, lv_pct(100));
        lv_obj_add_event_cb(btn, event_handler_select, LV_EVENT_CLICKED, (void *)&profile);

        lv_obj_t *lab = lv_label_create(btn);
        Reflow::title(profile, buf, 128);
        lv_label_set_text(lab, buf);
    }

    lv_tabview_set_act(tabview, 2, LV_ANIM_ON);

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
