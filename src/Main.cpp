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
#include "toaster.h"

#define PIN_HEATER_RELAY 15
#define PIN_BACKLIGHT_CTRL 6
#define TEMP_HOT 37

char buf[BUFSIZ];

lv_obj_t *tabview;
Toaster toast;

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
Button sett_btn_bright_up;
lv_obj_t *sett_label_bright;
Button sett_btn_bright_down;

lv_obj_t *tab_abut;

const Reflow::ReflowProfile *select_profile = nullptr;
const Reflow::ReflowProfile *active_profile = nullptr;

PID TPID;
std::atomic_bool baking = false;
elapsedMicros time_et;
elapsedMillis poll_et;
double temp = 24, PIDOut, tempTarget;
char tempBuf[512];
const float Kp = 2, Ki = 5, Kd = 1;

NoticeBoard *nb;
Notice *pwr_noti, *hot_noti, *no_prof;

const char *tab_names[] = {
    "Controls",
    "Graph",
    "Solder Selection",
    "Settings",
    "Faults",
};

static int last_tab = -1;

static void tabview_new_tab(lv_event_t *e = 0) {
    LV_UNUSED(e);
    int curr = lv_tabview_get_tab_act(tabview);
    if (curr != last_tab)
        toast.toast(tab_names[curr]);
    last_tab = curr;
}

static void tabview_set_tab(int tab) {
    lv_tabview_set_act(tabview, tab, LV_ANIM_ON);
    tabview_new_tab();
}

void notice_no_profile(Notice *noti) {
    tabview_set_tab(2);
    toast.toast("Select a Profile");
}

void enableHeater(bool enable) {
    enable &= baking;
    digitalWriteFast(PIN_HEATER_RELAY, enable);
    if (enable)
        nb->pushNotice(pwr_noti, true);
    else
        nb->clearNotice(pwr_noti);
}

double TEST_pollTemp() {
    static float curr_accel = 0;
    if (PIDOut && baking) {
        curr_accel = min(curr_accel + 0.008 + (curr_accel > 0 ? 0.02 : 0), 5);
    } else {
        curr_accel = max(curr_accel - 0.05, -0.5);
    }

    return max(temp + curr_accel, 24);
}

double pollTemp() {
    // TODO: poll temp sensor here
    return TEST_pollTemp();
}

void set_reflow_profile(const Reflow::ReflowProfile *profile = nullptr) {
    if (!profile) {
        grph_graph->setTitle("No Profile");
        lv_label_set_text(ctrl_label_select, "No Profile Selected");
    } else {
        grph_graph->setMainData(profile->Xs, profile->Ys);
        Reflow::title(profile, buf, BUFSIZ);
        grph_graph->setTitle(buf);
    }
    active_profile = profile;
}

void begin_reflow(const Reflow::ReflowProfile *profile) {
    time_et = 0;
    poll_et = 0;
    tempTarget = 0;
    PIDOut = 0;
    baking = true;
    // TODO: system check here
}

// safely stop reflow
void stop_reflow() {
    enableHeater(0);
    set_reflow_profile();
    ctrl_btn_start.disable(true);
    ctrl_btn_pause.disable(true);
    baking = false; // TODO: set delay on enabling baking, let system cool down first
}

void abort_reflow() { // TODO: take extra safety precautions here
    stop_reflow();
}

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
        set_reflow_profile(select_profile);
        ctrl_btn_start.disable(false);
        Reflow::Timing peak = Reflow::getPeak(active_profile);
        Reflow::Timing last = Reflow::getLast(active_profile);
        int cnt = Reflow::title(active_profile, buf, BUFSIZ);
        char *_buf = buf + cnt;
        snprintf(_buf, BUFSIZ - cnt, "\nPeak: %dC°@%ds\nETA: %ds", std::get<1>(peak), std::get<2>(peak), std::get<1>(last));
        lv_label_set_text(ctrl_label_select, buf);
        tabview_set_tab(0);
        toast.toast("Profile Loaded");
    }
}

static void ctrl_event_btn_start(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (!baking && active_profile != nullptr && code == LV_EVENT_CLICKED) {
        ctrl_btn_start.disable(true);
        ctrl_btn_pause.disable(false);
        begin_reflow(active_profile);
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
        abort_reflow();
    } else if (code == LV_EVENT_PRESSED) {
        lv_label_set_text(ctrl_label_select, "ABORT");
    }
}

static void sett_event_btn_bright(lv_event_t *e) {
    static int brightness = 100;
    lv_event_code_t code = lv_event_get_code(e);
    int bright_up = &sett_btn_bright_up == (Button *)lv_event_get_user_data(e);
    bright_up = bright_up + (!bright_up * -1);
    if (code == LV_EVENT_PRESSING) {
        brightness = clamp(brightness + bright_up, 0, 100);
        analogWrite(PIN_BACKLIGHT_CTRL, cmap(brightness, 0, 100, 1, 4096)); // TODO: Analog resolution
        lv_label_set_text_fmt(sett_label_bright, "%i", brightness);
    }
}

// TODO: look ahead with PID

void reset() {
    SCB_AIRCR = 0x05FA0004;
    // splashPlay(24, 1000);
}

int main(void) {
    // TODO: ensure safe state on reset
    pinMode(16, INPUT_PULLUP);
    analogWriteResolution(12);
    analogReadResolution(12);
    attachInterrupt(16, reset, LOW);
    Serial.begin(9600);
    Display::init();

    // TODO: add tabview fade in/out title
    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_RIGHT, lv_pct(10));
    lv_obj_set_style_bg_color(tabview, lv_palette_darken(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_text_color(tabview, lv_palette_lighten(LV_PALETTE_GREY, 4), 0);

    toast = Toaster(lv_scr_act());
    toast.setPos(16, Display::Height - 48);
    toast.setColor(LV_PALETTE_GREY);
    toast.setFont(&lv_font_montserrat_16);

    lv_obj_add_event_cb(tabview, tabview_new_tab, LV_EVENT_VALUE_CHANGED, 0);

    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_style_bg_color(tab_btns, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_text_color(tab_btns, lv_palette_lighten(LV_PALETTE_GREY, 5), 0);
    lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_LEFT, (lv_style_selector_t)LV_PART_ITEMS | (lv_style_selector_t)LV_STATE_CHECKED);

    tab_ctrl = lv_tabview_add_tab(tabview, LV_SYMBOL_HOME); // Controls
    // lv_obj_set_user_data(tab_ctrl, (void *)"Controls");
    tab_grph = lv_tabview_add_tab(tabview, LV_SYMBOL_IMAGE); // Graph
    // lv_obj_set_user_data(tab_grph, (void *)"Graph");
    tab_selc = lv_tabview_add_tab(tabview, LV_SYMBOL_LIST); // Paste Select
    // lv_obj_set_user_data(tab_selc, (void *)"Solder Selection");
    tab_sett = lv_tabview_add_tab(tabview, LV_SYMBOL_SETTINGS); // Settings
    // lv_obj_set_user_data(tab_sett, (void *)"Settings");
    tab_abut = lv_tabview_add_tab(tabview, LV_SYMBOL_WARNING); // About
    // lv_obj_set_user_data(tab_abut, (void *)"Faults");

    // Global
    NoticeBoard _nb(lv_scr_act(), 8);
    nb = &_nb;
    pwr_noti = nb->addNotice(LV_SYMBOL_CHARGE, 0, LV_PALETTE_YELLOW);
    hot_noti = nb->addNotice(LV_SYMBOL_WARNING, 0, LV_PALETTE_RED);               // TODO: fire symbol
    no_prof = nb->addNotice(LV_SYMBOL_IMAGE, notice_no_profile, LV_PALETTE_GREY); // TODO: paste symbol

    // Controls Tab
    ctrl_label_select = lv_label_create(tab_ctrl);
    lv_obj_set_style_text_font(ctrl_label_select, &lv_font_montserrat_16, 0);
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
        Reflow::title(&profile, buf, BUFSIZ);
        lv_label_set_text(lab, buf);
    }

    // Settings Tab
    sett_btn_bright_up = Button(tab_sett);
    sett_btn_bright_up.setLabel(LV_SYMBOL_UP);
    sett_btn_bright_up.setAlign(LV_ALIGN_TOP_MID);
    sett_btn_bright_up.setOnPressing(sett_event_btn_bright, &sett_btn_bright_up);
    sett_btn_bright_down = Button(tab_sett);
    sett_btn_bright_down.setLabel(LV_SYMBOL_DOWN);
    sett_btn_bright_down.setAlign(LV_ALIGN_BOTTOM_MID);
    sett_btn_bright_down.setOnPressing(sett_event_btn_bright, &sett_btn_bright_down);
    sett_label_bright = lv_label_create(tab_sett);
    lv_obj_set_style_text_font(sett_label_bright, &lv_font_montserrat_16, 0);
    lv_label_set_text(sett_label_bright, "100");
    lv_obj_align(sett_label_bright, LV_ALIGN_CENTER, 0, 0);

    TPID.Init(&temp, &PIDOut, &tempTarget, Kp, Ki, Kd, _PID_P_ON_E, _PID_CD_DIRECT);
    TPID.SetMode(_PID_MODE_AUTOMATIC);
    TPID.SetSampleTime(100);
    TPID.SetOutputLimits(0, 1); // binary, no PWM control // TODO: fan/exhaust control? [-1,1]

    pinMode(PIN_HEATER_RELAY, OUTPUT);

    set_reflow_profile();

    while (true) {
        double time = time_et / 1000000.0;
        if (poll_et >= 100) {
            poll_et -= 100;

            temp = pollTemp();

            if (baking && active_profile != nullptr) {
                tempTarget = grph_graph->updateData(time, temp);
                enableHeater((bool)PIDOut);
                TPID.Compute();
                Reflow::stateString(active_profile, temp, tempTarget, time, buf, BUFSIZ);
                grph_graph->setSubText(buf);
                if (time > std::get<1>(Reflow::getLast(active_profile))) // IMPROVE: better stop logic?
                    stop_reflow();
            } else {
                enableHeater(0);
                grph_graph->setSubText("");
            }

            if (temp > TEMP_HOT) {
                sprintf(buf, "%iC°", (int)temp);
                hot_noti->setLabel(buf);
                nb->pushNotice(hot_noti);
            } else {
                nb->clearNotice(hot_noti);
            }
            if (!active_profile) {
                nb->pushNotice(no_prof);
            } else {
                nb->clearNotice(no_prof);
            }
        }
        nb->update();
        lv_task_handler();
    }

    return 0;
}
