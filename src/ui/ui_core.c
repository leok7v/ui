#include "rt/rt.h"
#include "ui/ui.h"
#include "rt/rt_win32.h"

#define UI_WM_ANIMATE  (WM_APP + 0x7FFF)
#define UI_WM_OPENING  (WM_APP + 0x7FFE)
#define UI_WM_CLOSING  (WM_APP + 0x7FFD)
#define UI_WM_TAP      (WM_APP + 0x7FFC)
#define UI_WM_DTAP     (WM_APP + 0x7FFB) // double tap (aka click)
#define UI_WM_PRESS    (WM_APP + 0x7FFA)

static bool ui_point_in_rect(const ui_point_t* p, const ui_rect_t* r) {
    return r->x <= p->x && p->x < r->x + r->w &&
           r->y <= p->y && p->y < r->y + r->h;
}

static bool ui_intersect_rect(ui_rect_t* i, const ui_rect_t* r0,
                                            const ui_rect_t* r1) {
    ui_rect_t r = {0};
    r.x = rt_max(r0->x, r1->x);  // Maximum of left edges
    r.y = rt_max(r0->y, r1->y);  // Maximum of top edges
    r.w = rt_min(r0->x + r0->w, r1->x + r1->w) - r.x;  // Width of overlap
    r.h = rt_min(r0->y + r0->h, r1->y + r1->h) - r.y;  // Height of overlap
    bool b = r.w > 0 && r.h > 0;
    if (!b) {
        r.w = 0;
        r.h = 0;
    }
    if (i != null) { *i = r; }
    return b;
}

static ui_rect_t ui_combine_rect(const ui_rect_t* r0, const ui_rect_t* r1) {
    return (ui_rect_t) {
        .x = rt_min(r0->x, r1->x),
        .y = rt_min(r0->y, r1->y),
        .w = rt_max(r0->x + r0->w, r1->x + r1->w) - rt_min(r0->x, r1->x),
        .h = rt_max(r0->y + r0->h, r1->y + r1->h) - rt_min(r0->y, r1->y)
    };
}

ui_if ui = {
    .point_in_rect  = ui_point_in_rect,
    .intersect_rect = ui_intersect_rect,
    .combine_rect   = ui_combine_rect,
    .infinity = INT32_MAX,
    .align = {
        .center = 0,
        .left   = 0x01,
        .top    = 0x02,
        .right  = 0x10,
        .bottom = 0x20
    },
    .visibility = { // window visibility see ShowWindow link below
        .hide      = SW_HIDE,
        .normal    = SW_SHOWNORMAL,
        .minimize  = SW_SHOWMINIMIZED,
        .maximize  = SW_SHOWMAXIMIZED,
        .normal_na = SW_SHOWNOACTIVATE,
        .show      = SW_SHOW,
        .min_next  = SW_MINIMIZE,
        .min_na    = SW_SHOWMINNOACTIVE,
        .show_na   = SW_SHOWNA,
        .restore   = SW_RESTORE,
        .defau1t   = SW_SHOWDEFAULT,
        .force_min = SW_FORCEMINIMIZE
    },
    .message = {
        .animate               = UI_WM_ANIMATE,
        .opening               = UI_WM_OPENING,
        .closing               = UI_WM_CLOSING
    },
    .mouse = {
        .button = {
            .left  = MK_LBUTTON,
            .right = MK_RBUTTON
        }
    },
    .hit_test = {
        .error             = HTERROR,
        .transparent       = HTTRANSPARENT,
        .nowhere           = HTNOWHERE,
        .client            = HTCLIENT,
        .caption           = HTCAPTION,
        .system_menu       = HTSYSMENU,
        .grow_box          = HTGROWBOX,
        .menu              = HTMENU,
        .horizontal_scroll = HTHSCROLL,
        .vertical_scroll   = HTVSCROLL,
        .min_button        = HTMINBUTTON,
        .max_button        = HTMAXBUTTON,
        .left              = HTLEFT,
        .right             = HTRIGHT,
        .top               = HTTOP,
        .top_left          = HTTOPLEFT,
        .top_right         = HTTOPRIGHT,
        .bottom            = HTBOTTOM,
        .bottom_left       = HTBOTTOMLEFT,
        .bottom_right      = HTBOTTOMRIGHT,
        .border            = HTBORDER,
        .object            = HTOBJECT,
        .close             = HTCLOSE,
        .help              = HTHELP
    },
    .key = {
        .up        = VK_UP,
        .down      = VK_DOWN,
        .left      = VK_LEFT,
        .right     = VK_RIGHT,
        .home      = VK_HOME,
        .end       = VK_END,
        .page_up   = VK_PRIOR,
        .page_down = VK_NEXT,
        .insert    = VK_INSERT,
        .del       = VK_DELETE,
        .back      = VK_BACK,
        .escape    = VK_ESCAPE,
        .enter     = VK_RETURN,
        .minus     = VK_OEM_MINUS,
        .plus      = VK_OEM_PLUS,
        .f1        = VK_F1,
        .f2        = VK_F2,
        .f3        = VK_F3,
        .f4        = VK_F4,
        .f5        = VK_F5,
        .f6        = VK_F6,
        .f7        = VK_F7,
        .f8        = VK_F8,
        .f9        = VK_F9,
        .f10       = VK_F10,
        .f11       = VK_F11,
        .f12       = VK_F12,
        .f13       = VK_F13,
        .f14       = VK_F14,
        .f15       = VK_F15,
        .f16       = VK_F16,
        .f17       = VK_F17,
        .f18       = VK_F18,
        .f19       = VK_F19,
        .f20       = VK_F20,
        .f21       = VK_F21,
        .f22       = VK_F22,
        .f23       = VK_F23,
        .f24       = VK_F24,
    },
    .beep = {
        .ok         = 0,
        .info       = 1,
        .question   = 2,
        .warning    = 3,
        .error      = 4
    }
};

// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
