#include "Arduino.h"
#include "pid.h"
#include "spline.h"
#include <atomic>
#include <vector>

#include "animation.h"
#include "button.h"
#include "display.h"
#include "graph.h"
#include "notice.h"
#include "reflow.h"

#define STR_BUF_SZ 256

char buf[STR_BUF_SZ];

lv_obj_t *tabview;

lv_obj_t *tab_ctrl;
lv_obj_t *ctrl_label_select;
Button ctrl_btn_start;
Button ctrl_btn_pause;
std::atomic_bool ctrl_btn_pause_play = false;
Button ctrl_btn_abort;

lv_obj_t *tab_grph;
Graph::Graph *grph_graph;

lv_obj_t *tab_selc;
Graph::Graph *selc_graph;
Button selc_btn_confirm;
lv_obj_t *selc_list;
lv_obj_t *selc_active_btn = nullptr;

lv_obj_t *tab_sett;
lv_obj_t *tab_abut;

const Reflow::ReflowProfile *select_profile = nullptr;
const Reflow::ReflowProfile *active_profile = nullptr;

PID TPID;
std::atomic_bool baking = false;
elapsedMicros time_et;
elapsedMillis poll_et, label_et;
double temp, tempTarget, tempDV;
char tempBuf[512];
const char Kp = 2, Ki = 5, Kd = 1;

static void selc_event_list_select(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED && !baking) {
        if (selc_active_btn == obj) {
            selc_active_btn = nullptr;
            selc_btn_confirm.disable(true);
            selc_graph->setVisible(false);
        } else {
            selc_active_btn = obj;
            selc_btn_confirm.disable(false);
            selc_graph->setVisible(true);
        }
        lv_obj_t *parent = lv_obj_get_parent(obj);
        for (uint32_t i = 0; i < lv_obj_get_child_cnt(parent); i++) {
            lv_obj_t *child = lv_obj_get_child(parent, i);
            if (child == selc_active_btn) {
                lv_obj_add_state(child, LV_STATE_CHECKED);
                select_profile = (const Reflow::ReflowProfile *)lv_event_get_user_data(e);
                selc_graph->setMainData(select_profile->Xs, select_profile->Ys);
            } else {
                lv_obj_clear_state(child, LV_STATE_CHECKED);
            }
        }
    }
}

static void selc_event_btn_confirm(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED && select_profile != nullptr && !baking) {
        active_profile = select_profile;
        ctrl_btn_start.disable(false);
        Reflow::Timing peak = Reflow::getPeak(active_profile);
        Reflow::Timing last = Reflow::getLast(active_profile);
        int cnt = Reflow::title(active_profile, buf, STR_BUF_SZ);
        char *_buf = buf + cnt;
        snprintf(_buf, STR_BUF_SZ - cnt, "\nPeak: %dC°@%ds\nETA: %ds", std::get<1>(peak), std::get<2>(peak), std::get<1>(last));
        lv_label_set_text(ctrl_label_select, buf);
        lv_tabview_set_act(tabview, 0, LV_ANIM_ON);
    }
}
void begin_baking(const Reflow::ReflowProfile *profile) {
    baking = true;
    // TODO: system check here
    grph_graph->setMainData(profile->Xs, profile->Ys);
    Reflow::title(profile, buf, STR_BUF_SZ);
    grph_graph->setTitle(buf);
}
void abort_baking() {
    baking = false; // TODO: set delay on enabling baking
    grph_graph->setTitle("No Profile");
}
static void ctrl_event_btn_start(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (!baking && active_profile != nullptr && code == LV_EVENT_CLICKED) {
        ctrl_btn_start.disable(true);
        ctrl_btn_pause.disable(false);
        begin_baking(active_profile);
    }
}
static void ctrl_event_btn_pause(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (baking && active_profile != nullptr && code == LV_EVENT_CLICKED) {
        // TODO: pause function
        if (!ctrl_btn_pause_play) {
            ctrl_btn_pause.setLabel(LV_SYMBOL_PLAY);
            ctrl_btn_pause.setColor(LV_PALETTE_BLUE);
        } else {
            ctrl_btn_pause.setLabel(LV_SYMBOL_PAUSE);
            ctrl_btn_pause.setColor(LV_PALETTE_AMBER);
        }
        ctrl_btn_pause_play = !ctrl_btn_pause_play;
    }
}
static void ctrl_event_btn_abort(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED || code == LV_EVENT_RELEASED) { // Will trigger if touched in anyway
        // TODO: reset ui func
        ctrl_btn_start.disable(true);
        ctrl_btn_pause.disable(true);
        lv_label_set_text(ctrl_label_select, "No Profile Selected");
        active_profile = nullptr;
        abort_baking();
    } else if (code == LV_EVENT_PRESSED) {
        lv_label_set_text(ctrl_label_select, "ABORT");
    }
}

// TODO: Heat warning, show when temp is detected as hot
// TODO: show any issues as tab on left side
// TODO: Time to peak

double pollTemp() {
    // TODO: poll temp sensor here
    return 0;
}

double TEST_pollTemp(double time = random(300)) {
    static const Reflow::ReflowProfile *set_profile;
    static tk::spline spln;
    static int32_t rnd = 0;
    if (active_profile != nullptr) {
        if (set_profile != active_profile) {
            set_profile = active_profile;
            spln = tk::spline(set_profile->Xs, set_profile->Ys, tk::spline::cspline_hermite);
            time_et = 0;
        }
        return spln(time) + (rnd += random(-4, 5));
    }
    return 0;
}

void notice_test_cb(notice *noti) {
    lv_tabview_set_act(tabview, 3, LV_ANIM_ON);
    noti->jiggle(1000, 15);
}

int main(void) {
    Display::init();

    // TODO: add tabview fade in/out title
    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_RIGHT, lv_pct(10));
    lv_obj_set_style_bg_color(tabview, lv_palette_darken(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_text_color(tabview, lv_palette_lighten(LV_PALETTE_GREY, 4), 0);

    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_style_bg_color(tab_btns, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_text_color(tab_btns, lv_palette_lighten(LV_PALETTE_GREY, 5), 0);
    lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_LEFT, LV_PART_ITEMS | LV_STATE_CHECKED);

    tab_ctrl = lv_tabview_add_tab(tabview, LV_SYMBOL_HOME);     // Controls
    tab_grph = lv_tabview_add_tab(tabview, LV_SYMBOL_IMAGE);    // Graph
    tab_selc = lv_tabview_add_tab(tabview, LV_SYMBOL_LIST);     // Paste Select
    tab_sett = lv_tabview_add_tab(tabview, LV_SYMBOL_SETTINGS); // Settings
    tab_abut = lv_tabview_add_tab(tabview, LV_SYMBOL_WARNING);  // About

    // Global

    notice noti(lv_scr_act(), notice_test_cb);
    noti.setLabel(LV_SYMBOL_POWER);
    noti.setColor(LV_PALETTE_RED);
    noti.setHeight(45);

    // Controls Tab
    ctrl_label_select = lv_label_create(tab_ctrl);
    lv_obj_set_style_text_font(ctrl_label_select, &lv_font_montserrat_16, 0);
    lv_label_set_text(ctrl_label_select, "No Profile Selected");
    lv_obj_align(ctrl_label_select, LV_ALIGN_TOP_LEFT, 0, 0);

    ctrl_btn_start = Button(tab_ctrl);
    ctrl_btn_start.disable(true);
    ctrl_btn_start.setSize(lv_pct(30), lv_pct(30));
    ctrl_btn_start.setAlign(LV_ALIGN_BOTTOM_LEFT);
    ctrl_btn_start.setColor(LV_PALETTE_GREEN);
    ctrl_btn_start.setOnClick(ctrl_event_btn_start);
    ctrl_btn_start.setLabel(LV_SYMBOL_OK);
    ctrl_btn_start.setFont(&lv_font_montserrat_24);

    ctrl_btn_pause = Button(tab_ctrl);
    ctrl_btn_pause.disable(true);
    ctrl_btn_pause.setSize(lv_pct(30), lv_pct(30));
    ctrl_btn_pause.setAlign(LV_ALIGN_BOTTOM_MID, -13);
    ctrl_btn_pause.setColor(LV_PALETTE_AMBER);
    ctrl_btn_pause.setOnClick(ctrl_event_btn_pause);
    ctrl_btn_pause.setLabel(LV_SYMBOL_PAUSE);
    ctrl_btn_pause.setFont(&lv_font_montserrat_24);

    ctrl_btn_abort = Button(tab_ctrl);
    ctrl_btn_abort.setSize(lv_pct(40), lv_pct(30));
    ctrl_btn_abort.setAlign(LV_ALIGN_BOTTOM_RIGHT);
    ctrl_btn_abort.setColor(LV_PALETTE_RED);
    ctrl_btn_abort.setOnClick(ctrl_event_btn_abort);
    ctrl_btn_abort.setOnDown(ctrl_event_btn_abort);
    ctrl_btn_abort.setLabel(LV_SYMBOL_STOP);
    ctrl_btn_abort.setFont(&lv_font_montserrat_24);

    // Graph Tab
    Graph::Graph g(tab_grph);
    grph_graph = &g;
    grph_graph->setSize(lv_pct(90), lv_pct(85));
    grph_graph->setPos(-20, -2);
    grph_graph->setSubText("");
    grph_graph->setTitle("No Profile");

    // Select Tab
    Graph::Graph pg(tab_selc, true);
    selc_graph = &pg;
    selc_graph->setSize(lv_pct(80), lv_pct(40));
    selc_graph->align(LV_ALIGN_TOP_LEFT);

    selc_btn_confirm = Button(tab_selc);
    selc_btn_confirm.disable(true);
    selc_btn_confirm.setSize(lv_pct(20), lv_pct(35));
    selc_btn_confirm.setAlign(LV_ALIGN_TOP_RIGHT);
    selc_btn_confirm.setColor(LV_PALETTE_GREEN);
    selc_btn_confirm.setOnClick(selc_event_btn_confirm);
    selc_btn_confirm.setLabel(LV_SYMBOL_OK);
    selc_btn_confirm.setFont(&lv_font_montserrat_24);

    selc_list = lv_list_create(tab_selc);
    lv_obj_set_size(selc_list, lv_pct(100), lv_pct(60));
    lv_obj_set_style_pad_row(selc_list, 5, 0);
    lv_obj_set_style_bg_opa(selc_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(selc_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(selc_list, 0, 0);
    lv_obj_set_style_outline_pad(selc_list, 0, 0);
    lv_obj_align(selc_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_scrollbar_mode(selc_list, LV_SCROLLBAR_MODE_ACTIVE);

    for (auto &profile : Reflow::PROFILES) {
        lv_obj_t *btn = lv_btn_create(selc_list);
        lv_obj_set_width(btn, lv_pct(100));
        lv_obj_add_event_cb(btn, selc_event_list_select, LV_EVENT_CLICKED, (void *)&profile);

        lv_obj_t *lab = lv_label_create(btn);
        Reflow::title(&profile, buf, STR_BUF_SZ);
        lv_label_set_text(lab, buf);
    }

    TPID.Init(&temp, &tempDV, &tempTarget, Kp, Ki, Kd, _PID_P_ON_E, _PID_CD_DIRECT);
    TPID.SetMode(_PID_MODE_AUTOMATIC);
    TPID.SetSampleTime(100);
    TPID.SetOutputLimits(0, 300);

    while (true) {
        double time = time_et / 1000000.0;
        if (poll_et >= 100) {
            poll_et -= 100;
            temp = TEST_pollTemp(time);
            if (baking && active_profile != nullptr) {
                grph_graph->updateData(time, temp);
                if (label_et >= 500) {
                    label_et = 0;
                    Reflow::stateString(active_profile, temp, time, buf, STR_BUF_SZ);
                    grph_graph->setSubText(buf);
                }
            } else {
                grph_graph->setSubText("");
            }
            TPID.Compute();
            if (random(100) > 98) {
                noti.jiggle();
            }
        }
        lv_task_handler();
    }

    return 0;
}
