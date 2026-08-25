#include "../winmini.h"

static HMODULE g_self = NULL;
int _fltused = 0;

/* Keep a real base relocation in the image so ASLR can move this DLL safely. */
__declspec(dllexport) void* g_luvvy_menu_reloc_anchor = (void*)&g_self;
static HWND g_game_window = NULL;
static HWND g_overlay_window = NULL;
static BYTE* g_game_base = NULL;
static volatile BOOL g_running = TRUE;
static BOOL g_visible = FALSE;
static BOOL g_offsets_ready = FALSE;
static BOOL g_logged_verified = FALSE;
static BOOL g_f10_down = FALSE;
static BOOL g_end_down = FALSE;
static BOOL g_prev_down = FALSE;
static BOOL g_next_down = FALSE;
static BOOL g_action_down = FALSE;
static BOOL g_f9_down = FALSE;
static BOOL g_advanced_view = FALSE;
static const int MENU_W = 720;
static const int MENU_H = 650;
static int g_selected = 0;

/* Exact Steam 1.0.0.2 build guard from the user supplied executable. */
#define TARGET_PE_TIMESTAMP 0x669E384AUL
#define TARGET_IMAGE_SIZE   0x02C2B000UL

/* Steam 1.0.0.2 RVAs. */
#define RVA_SAVE       0x09A98B0ULL
#define RVA_NOW        0x0717008ULL
#define RVA_GAME_SPEED   0x0717424ULL
#define RVA_SLOT1        0x2A23598ULL
#define RVA_DEBUG_FLAGS  0x0749804ULL
#define RVA_GAME_TIME    0x09ABCF4ULL

/* Additional Steam 1.0.0.2 globals cross-checked against the supplied CE table. */
#define RVA_SAVE_PTR              0x009A0778ULL
#define RVA_PLAYER_PTR            0x02A23790ULL
#define RVA_MOTION_PTR            0x00718CB0ULL
#define RVA_CAMERA_PTR            0x00B0D540ULL
#define RVA_BASE_SPEED_PTR        0x02AE5780ULL
#define RVA_MINIGAME_TIME         0x00ABB850ULL
#define RVA_MINIGAME_TIME_START   0x00ABB858ULL
#define RVA_STRUGGLE_MY_ORBS      0x02A0FA48ULL
#define RVA_STRUGGLE_FOE_ORBS     0x02A0FA98ULL
#define RVA_GRANDSTANDER_COMBO    0x02A0FC58ULL
#define RVA_GRANDSTANDER_MAX      0x02A0FC70ULL

/* CE instruction sites verified against the user supplied Steam executable. */
#define RVA_CE_ONE_HIT_KILL       0x003C0870ULL
#define RVA_CE_INSTANT_HASTE      0x003D6986ULL
#define RVA_CE_FREE_COMMAND       0x00402163ULL

#define OFF_CE_MUNNY              0x35C0ULL
#define OFF_CE_EXP                0x4860ULL
#define OFF_CE_FORM               0x46A4ULL
#define OFF_DRIVE_FORM_CURRENT    0x1B4ULL
#define OFF_DRIVE_FORM_MAX        0x1B8ULL
#define OFF_PLAYER_SPEED          0x12CULL
#define OFF_PLAYER_JUMP           0x130ULL
#define OFF_PLAYER_TURN           0x134ULL
#define OFF_PLAYER_WEIGHT         0x138ULL
#define OFF_PLAYER_Y              0x670ULL
#define OFF_PLAYER_Z              0x674ULL
#define OFF_PLAYER_X              0x678ULL
#define OFF_PLAYER_ANGLE          0x68CULL
#define OFF_CAMERA_ANGLE          0x2330ULL

#define OFF_HP_CURRENT     0x000ULL
#define OFF_HP_MAX         0x004ULL
#define OFF_MP_CURRENT     0x180ULL
#define OFF_MP_MAX         0x184ULL
#define OFF_DRIVE_PERCENT  0x1B0ULL
#define OFF_DRIVE_CURRENT  0x1B1ULL
#define OFF_DRIVE_MAX      0x1B2ULL
#define OFF_MUNNY          0x2440ULL
#define OFF_FORM_UNLOCKS   0x36C0ULL
#define OFF_FORM_LEVEL     0x32F6ULL
#define FORM_LEVEL_STRIDE  0x38ULL

#define ITEM_INF_HP 0
#define ITEM_INF_MP 1
#define ITEM_INF_DRIVE 2
#define ITEM_SPEED_HALF 3
#define ITEM_SPEED_DOUBLE 4
#define ITEM_SPEED_QUAD 5
#define ITEM_MUNNY_LOCK 6
#define ITEM_DRIVE_MAX9 7
#define ITEM_UNLOCK_FORMS 8
#define ITEM_MAX_FORM_LEVELS 9
#define ITEM_REFILL_NOW 10
#define ITEM_FREEZE_GAME_TIME 11
#define ITEM_SQUARE_FIRST 12
#define ITEM_SQUARE_LAST 39
#define ITEM_DISABLE_ALL 40
#define ITEM_INF_DRIVE_FORM 41
#define ITEM_EXP_MULTIPLIER 42
#define ITEM_MUNNY_MULTIPLIER 43
#define ITEM_TURBO_MOVEMENT 44
#define ITEM_FLY_MODE 45
#define ITEM_LOW_GRAVITY 46
#define ITEM_FREEZE_MINIGAME_TIMERS 47
#define ITEM_WIN_STRUGGLE 48
#define ITEM_GRANDSTANDER_COMBO 49
#define ITEM_ACTIVATE_ALL_ABILITIES 50
#define ITEM_ONE_HIT_KILL 51
#define ITEM_INSTANT_HASTE 52
#define ITEM_FREE_COMMAND 53
#define ITEM_COUNT 54
#define VISIBLE_ITEMS 12
#define SQUARE_FLAG_COUNT 28

static BOOL g_inf_hp = FALSE;
static BOOL g_inf_mp = FALSE;
static BOOL g_inf_drive = FALSE;
static int g_speed_mode = 0;
static BOOL g_speed_original_valid = FALSE;
static float g_speed_original = 1.0f;
static BOOL g_munny_lock = FALSE;
static BOOL g_munny_original_valid = FALSE;
static int g_munny_original = 0;
static BOOL g_drive_max9 = FALSE;
static BOOL g_drive_original_valid = FALSE;
static BYTE g_drive_percent_original = 0;
static BYTE g_drive_current_original = 0;
static BYTE g_drive_max_original = 0;
static BOOL g_unlock_forms = FALSE;
static BOOL g_forms_original_valid = FALSE;
static BYTE g_forms_original = 0;
static BOOL g_max_form_levels = FALSE;
static BOOL g_form_levels_original_valid = FALSE;
static BYTE g_form_levels_original[5] = {0,0,0,0,0};
static BOOL g_freeze_game_time = FALSE;
static DWORD g_frozen_game_time = 0;
static BOOL g_debug_flags_original_valid = FALSE;
static DWORD g_debug_flags_original = 0;

/* CE-derived modules. These are isolated from the frozen loader/overlay core. */
static BOOL g_inf_drive_form = FALSE;
static int g_exp_multiplier = 1;
static BOOL g_exp_prev_valid = FALSE;
static int g_exp_prev = 0;
static int g_munny_multiplier = 1;
static BOOL g_munny_prev_valid = FALSE;
static int g_munny_prev = 0;
static BOOL g_turbo_movement = FALSE;
static BOOL g_fly_mode = FALSE;
static BOOL g_low_gravity = FALSE;
static BOOL g_freeze_minigame_timers = FALSE;
static BOOL g_weight_original_valid = FALSE;
static BYTE* g_weight_object = NULL;
static float g_weight_original = 16.0f;
static BOOL g_all_abilities = FALSE;
static BOOL g_abilities_original_valid = FALSE;
static BYTE* g_abilities_save_base = NULL;
static BYTE g_abilities_original[79];

/* Isolated CE instruction hook/patch module. */
static BOOL g_one_hit_kill = FALSE;
static BOOL g_instant_haste = FALSE;
static BOOL g_free_command = FALSE;
static BYTE* g_ohk_cave = NULL;
static BYTE g_ohk_patch_bytes[6] = {0,0,0,0,0,0};
static BOOL g_ohk_patch_bytes_valid = FALSE;
static BOOL g_haste_original_valid = FALSE;
static BYTE* g_haste_object = NULL;
static float g_haste_original = 0.0f;

static const wchar_t* g_square_flag_labels[SQUARE_FLAG_COUNT] = {
    L"Square: Anytime Drive",
    L"Square: Anytime Magic",
    L"Square: R2 Map Debug",
    L"Square: Skip Title",
    L"Square: Ignore Zone",
    L"Square: Test Limit",
    L"Square: Exception  DANGER",
    L"Square: GM Save Local",
    L"Square: Stop Enemy",
    L"Square: Demo Movie",
    L"Square: Infinity Item",
    L"Square: Copyright",
    L"Square: Oni/Oshi",
    L"Square: Clear Cache  DANGER",
    L"Square: No Gameover",
    L"Square: Anytime Mickey",
    L"Square: Free Ability",
    L"Square: Gentle Friend",
    L"Square: Old Event Skip",
    L"Square: Sound Log",
    L"Square: No Check Effect Memory",
    L"Square: GM Item Max",
    L"Square: Memory Check  DANGER",
    L"Square: No Pack Read  DANGER",
    L"Square: Snapshot X2",
    L"Square: Show Version",
    L"Square: Caption Off",
    L"Square: Words Line On"
};

static unsigned int str_len_a(const char* s) {
    unsigned int n = 0;
    while (s && s[n]) ++n;
    return n;
}

static void path_dirname_w(wchar_t* path) {
    unsigned int i = 0, last = 0;
    while (path[i]) {
        if (path[i] == L'\\' || path[i] == L'/') last = i;
        ++i;
    }
    path[last + 1] = 0;
}

static void path_append_w(wchar_t* dst, const wchar_t* src, unsigned int cap) {
    unsigned int d = 0, s = 0;
    while (d + 1 < cap && dst[d]) ++d;
    while (d + 1 < cap && src[s]) dst[d++] = src[s++];
    dst[d] = 0;
}

static void write_log(const char* text) {
    wchar_t path[MAX_PATH];
    DWORD written = 0;
    path[0] = 0;
    if (!GetModuleFileNameW(g_self, path, MAX_PATH)) return;
    path_dirname_w(path);
    path_append_w(path, L"KH2ModMenu.log", MAX_PATH);
    HANDLE f = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return;
    WriteFile(f, text, str_len_a(text), &written, NULL);
    CloseHandle(f);
}

static void w_clear(wchar_t* dst) {
    dst[0] = 0;
}

static void w_append(wchar_t* dst, const wchar_t* src, unsigned int cap) {
    unsigned int d = 0, s = 0;
    while (d + 1 < cap && dst[d]) ++d;
    while (d + 1 < cap && src[s]) dst[d++] = src[s++];
    dst[d] = 0;
}

static void w_append_u32(wchar_t* dst, unsigned int value, unsigned int cap) {
    wchar_t tmp[16];
    unsigned int n = 0;
    if (value == 0) {
        w_append(dst, L"0", cap);
        return;
    }
    while (value && n < 15) {
        tmp[n++] = (wchar_t)(L'0' + (value % 10));
        value /= 10;
    }
    while (n) {
        wchar_t one[2];
        one[0] = tmp[--n];
        one[1] = 0;
        w_append(dst, one, cap);
    }
}

static void w_append_int_nonnegative(wchar_t* dst, int value, unsigned int cap) {
    if (value < 0) w_append(dst, L"?", cap);
    else w_append_u32(dst, (unsigned int)value, cap);
}

static BYTE mem_read_u8(unsigned long long rva) {
    return *(volatile BYTE*)(g_game_base + rva);
}

static int mem_read_i32(unsigned long long rva) {
    return *(volatile int*)(g_game_base + rva);
}

static DWORD mem_read_u32(unsigned long long rva) {
    return *(volatile DWORD*)(g_game_base + rva);
}

static float mem_read_f32(unsigned long long rva) {
    return *(volatile float*)(g_game_base + rva);
}

static unsigned long long mem_read_u64(unsigned long long rva) {
    return *(volatile unsigned long long*)(g_game_base + rva);
}

static BYTE* global_ptr(unsigned long long rva) {
    unsigned long long value;
    if (!g_offsets_ready || !g_game_base) return NULL;
    value = mem_read_u64(rva);
    if (value < 0x10000ULL || value > 0x00007FFFFFFFFFFFULL) return NULL;
    return (BYTE*)(ULONG_PTR)value;
}

static BYTE ptr_read_u8(BYTE* base, unsigned long long off) {
    return *(volatile BYTE*)(base + off);
}

static int ptr_read_i32(BYTE* base, unsigned long long off) {
    return *(volatile int*)(base + off);
}

static float ptr_read_f32(BYTE* base, unsigned long long off) {
    return *(volatile float*)(base + off);
}

static void ptr_write_u8(BYTE* base, unsigned long long off, BYTE value) {
    *(volatile BYTE*)(base + off) = value;
}

static void ptr_write_i32(BYTE* base, unsigned long long off, int value) {
    *(volatile int*)(base + off) = value;
}

static void ptr_write_f32(BYTE* base, unsigned long long off, float value) {
    *(volatile float*)(base + off) = value;
}

static void mem_write_u8(unsigned long long rva, BYTE value) {
    *(volatile BYTE*)(g_game_base + rva) = value;
}

static void mem_write_i32(unsigned long long rva, int value) {
    *(volatile int*)(g_game_base + rva) = value;
}

static void mem_write_u32(unsigned long long rva, DWORD value) {
    *(volatile DWORD*)(g_game_base + rva) = value;
}

static void mem_write_f32(unsigned long long rva, float value) {
    *(volatile float*)(g_game_base + rva) = value;
}

static BOOL verify_target_build(void) {
    BYTE* base = (BYTE*)GetModuleHandleW(NULL);
    if (!base) return FALSE;

    DWORD pe_offset = *(volatile DWORD*)(base + 0x3C);
    if (pe_offset < 0x40 || pe_offset > 0x1000) return FALSE;
    if (*(volatile DWORD*)(base + pe_offset) != 0x00004550UL) return FALSE;

    DWORD timestamp = *(volatile DWORD*)(base + pe_offset + 8);
    DWORD image_size = *(volatile DWORD*)(base + pe_offset + 0x50);
    if (timestamp != TARGET_PE_TIMESTAMP || image_size != TARGET_IMAGE_SIZE) return FALSE;

    if (base[RVA_SAVE + 0] != 'K' || base[RVA_SAVE + 1] != 'H' ||
        base[RVA_SAVE + 2] != '2' || base[RVA_SAVE + 3] != 'J') return FALSE;

    g_game_base = base;
    return TRUE;
}

static BOOL player_stats_valid(void) {
    if (!g_offsets_ready || !g_game_base) return FALSE;
    int max_hp = mem_read_i32(RVA_SLOT1 + OFF_HP_MAX);
    int max_mp = mem_read_i32(RVA_SLOT1 + OFF_MP_MAX);
    BYTE max_drive = mem_read_u8(RVA_SLOT1 + OFF_DRIVE_MAX);
    if (max_hp <= 0 || max_hp > 9999) return FALSE;
    if (max_mp < 0 || max_mp > 9999) return FALSE;
    if (max_drive > 20) return FALSE;
    return TRUE;
}

static void restore_speed(void) {
    if (g_offsets_ready && g_speed_original_valid) {
        mem_write_f32(RVA_GAME_SPEED, g_speed_original);
    }
    g_speed_mode = 0;
    g_speed_original_valid = FALSE;
}

static void restore_munny(void) {
    if (g_offsets_ready && g_munny_original_valid) {
        mem_write_i32(RVA_SAVE + OFF_MUNNY, g_munny_original);
    }
    g_munny_lock = FALSE;
    g_munny_original_valid = FALSE;
}

static void restore_drive_max(void) {
    if (g_offsets_ready && g_drive_original_valid) {
        mem_write_u8(RVA_SLOT1 + OFF_DRIVE_PERCENT, g_drive_percent_original);
        mem_write_u8(RVA_SLOT1 + OFF_DRIVE_CURRENT, g_drive_current_original);
        mem_write_u8(RVA_SLOT1 + OFF_DRIVE_MAX, g_drive_max_original);
    }
    g_drive_max9 = FALSE;
    g_drive_original_valid = FALSE;
}

static void restore_forms(void) {
    if (g_offsets_ready && g_forms_original_valid) {
        mem_write_u8(RVA_SAVE + OFF_FORM_UNLOCKS, g_forms_original);
    }
    g_unlock_forms = FALSE;
    g_forms_original_valid = FALSE;
}

static void restore_form_levels(void) {
    if (g_offsets_ready && g_form_levels_original_valid) {
        int i;
        for (i = 0; i < 5; ++i) {
            mem_write_u8(RVA_SAVE + OFF_FORM_LEVEL + FORM_LEVEL_STRIDE * (unsigned long long)i, g_form_levels_original[i]);
        }
    }
    g_max_form_levels = FALSE;
    g_form_levels_original_valid = FALSE;
}

static BOOL square_flag_enabled(int bit) {
    DWORD flags;
    if (!g_offsets_ready || bit < 0 || bit >= SQUARE_FLAG_COUNT) return FALSE;
    flags = mem_read_u32(RVA_DEBUG_FLAGS);
    return (flags & ((DWORD)1UL << bit)) != 0;
}

static void toggle_square_flag(int bit) {
    DWORD flags;
    DWORD mask;
    if (!g_offsets_ready || bit < 0 || bit >= SQUARE_FLAG_COUNT) return;

    flags = mem_read_u32(RVA_DEBUG_FLAGS);
    if (!g_debug_flags_original_valid) {
        g_debug_flags_original = flags;
        g_debug_flags_original_valid = TRUE;
    }

    mask = ((DWORD)1UL << bit);
    flags ^= mask;
    mem_write_u32(RVA_DEBUG_FLAGS, flags);
    write_log("[square] internal TEST_FLAG bit toggled\r\n");
}

static void restore_square_flags(void) {
    if (g_offsets_ready && g_debug_flags_original_valid) {
        mem_write_u32(RVA_DEBUG_FLAGS, g_debug_flags_original);
    }
    g_debug_flags_original_valid = FALSE;
}

static void toggle_freeze_game_time(void) {
    if (!g_offsets_ready) return;
    if (g_freeze_game_time) {
        g_freeze_game_time = FALSE;
        write_log("[cheat] game time freeze disabled\r\n");
        return;
    }
    g_frozen_game_time = mem_read_u32(RVA_GAME_TIME);
    g_freeze_game_time = TRUE;
    write_log("[cheat] game time freeze enabled\r\n");
}


static int cycle_multiplier(int value) {
    if (value <= 1) return 2;
    if (value == 2) return 4;
    if (value == 4) return 8;
    return 1;
}

static BOOL float_sane(float value, float min_value, float max_value) {
    return value >= min_value && value <= max_value;
}

static void toggle_inf_drive_form(void) {
    g_inf_drive_form = !g_inf_drive_form;
    write_log(g_inf_drive_form ? "[cheat] infinite drive form duration enabled\r\n" : "[cheat] infinite drive form duration disabled\r\n");
}

static void cycle_exp_multiplier(void) {
    g_exp_multiplier = cycle_multiplier(g_exp_multiplier);
    g_exp_prev_valid = FALSE;
    write_log("[cheat] EXP gain multiplier cycled\r\n");
}

static void cycle_munny_multiplier(void) {
    g_munny_multiplier = cycle_multiplier(g_munny_multiplier);
    g_munny_prev_valid = FALSE;
    write_log("[cheat] munny gain multiplier cycled\r\n");
}

static int movement_form(void) {
    BYTE* save = global_ptr(RVA_SAVE_PTR);
    if (!save) return -1;
    {
        int form = (int)ptr_read_u8(save, OFF_CE_FORM);
        if (form < 0 || form > 6) return -1;
        return form;
    }
}

static BOOL base_movement_for_form(int form, float* speed, float* jump, float* turn) {
    BYTE* table = global_ptr(RVA_BASE_SPEED_PTR);
    unsigned long long off = 0;
    if (!table) return FALSE;
    switch (form) {
        case 0: off = 0x120ULL; break;
        case 1: off = 0x154ULL; break;
        case 2: off = 0x188ULL; break;
        case 3: off = 0x7A0ULL; break;
        case 4: off = 0x1BCULL; break;
        case 5: off = 0x1F0ULL; break;
        case 6: off = 0x224ULL; break;
        default: return FALSE;
    }
    *speed = ptr_read_f32(table, off + 0);
    *jump = ptr_read_f32(table, off + 4);
    *turn = ptr_read_f32(table, off + 8);
    return float_sane(*speed, 0.01f, 200.0f) && float_sane(*jump, 0.01f, 5000.0f) && float_sane(*turn, 0.001f, 10.0f);
}

static void restore_turbo_movement(void) {
    BYTE* player;
    if (!g_turbo_movement) return;
    player = global_ptr(RVA_MOTION_PTR);
    float speed, jump, turn;
    int form = movement_form();
    if (player && base_movement_for_form(form, &speed, &jump, &turn)) {
        ptr_write_f32(player, OFF_PLAYER_SPEED, speed);
        ptr_write_f32(player, OFF_PLAYER_JUMP, jump);
        ptr_write_f32(player, OFF_PLAYER_TURN, turn);
    }
    g_turbo_movement = FALSE;
}

static void toggle_turbo_movement(void) {
    if (g_turbo_movement) {
        restore_turbo_movement();
        write_log("[cheat] CE custom movement boost disabled and current form defaults restored\r\n");
    } else {
        g_turbo_movement = TRUE;
        write_log("[cheat] CE custom movement boost enabled\r\n");
    }
}

static void capture_weight_if_needed(BYTE* player) {
    float current;
    if (!player) return;
    if (g_weight_original_valid && g_weight_object == player) return;
    current = ptr_read_f32(player, OFF_PLAYER_WEIGHT);
    if (!float_sane(current, -1000.0f, 1000.0f)) current = 16.0f;
    g_weight_object = player;
    g_weight_original = current;
    g_weight_original_valid = TRUE;
}

static void restore_weight_if_possible(void) {
    BYTE* player = global_ptr(RVA_MOTION_PTR);
    if (player && g_weight_original_valid && player == g_weight_object) {
        ptr_write_f32(player, OFF_PLAYER_WEIGHT, g_weight_original);
    }
    g_weight_original_valid = FALSE;
    g_weight_object = NULL;
}

static void toggle_fly_mode(void) {
    g_fly_mode = !g_fly_mode;
    if (!g_fly_mode && !g_low_gravity) restore_weight_if_possible();
    write_log(g_fly_mode ? "[cheat] fly mode enabled\r\n" : "[cheat] fly mode disabled\r\n");
}

static void toggle_low_gravity(void) {
    g_low_gravity = !g_low_gravity;
    if (!g_low_gravity && !g_fly_mode) restore_weight_if_possible();
    write_log(g_low_gravity ? "[cheat] low gravity enabled\r\n" : "[cheat] low gravity disabled\r\n");
}

static float wrap_pi(float x) {
    const float pi = 3.14159265358979323846f;
    const float two_pi = 6.28318530717958647692f;
    while (x > pi) x -= two_pi;
    while (x < -pi) x += two_pi;
    return x;
}

static float fast_sin(float x) {
    float x2;
    x = wrap_pi(x);
    x2 = x * x;
    return x * (1.0f - x2 * (0.16666667f - x2 * (0.00833333f - x2 * 0.00019841f)));
}

static float fast_cos(float x) {
    return fast_sin(x + 1.57079632679489661923f);
}

static void apply_fly_mode(void) {
    BYTE* player;
    BYTE* camera;
    float cam;
    float x, y, z;
    float step;
    float vertical;
    if (!g_fly_mode || g_visible) return;
    player = global_ptr(RVA_MOTION_PTR);
    camera = global_ptr(RVA_CAMERA_PTR);
    if (!player || !camera) return;

    capture_weight_if_needed(player);
    ptr_write_f32(player, OFF_PLAYER_WEIGHT, 0.0f);

    cam = ptr_read_f32(camera, OFF_CAMERA_ANGLE);
    x = ptr_read_f32(player, OFF_PLAYER_X);
    y = ptr_read_f32(player, OFF_PLAYER_Y);
    z = ptr_read_f32(player, OFF_PLAYER_Z);
    if (!float_sane(cam, -1000.0f, 1000.0f) || !float_sane(x, -10000000.0f, 10000000.0f) ||
        !float_sane(y, -10000000.0f, 10000000.0f) || !float_sane(z, -10000000.0f, 10000000.0f)) return;

    step = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 45.0f : 16.0f;
    vertical = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 36.0f : 18.0f;

    if (GetAsyncKeyState(VK_W) & 0x8000) {
        x += -step * fast_cos(cam);
        y += -step * fast_sin(cam);
        ptr_write_f32(player, OFF_PLAYER_ANGLE, cam + 3.14159265358979323846f);
    } else if (GetAsyncKeyState(VK_S) & 0x8000) {
        x += step * fast_cos(cam);
        y += step * fast_sin(cam);
        ptr_write_f32(player, OFF_PLAYER_ANGLE, cam);
    }
    if (GetAsyncKeyState(VK_D) & 0x8000) {
        x += -step * fast_cos(cam + 1.57079632679489661923f);
        y += -step * fast_sin(cam + 1.57079632679489661923f);
    } else if (GetAsyncKeyState(VK_A) & 0x8000) {
        x += step * fast_cos(cam + 1.57079632679489661923f);
        y += step * fast_sin(cam + 1.57079632679489661923f);
    }
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) z -= vertical;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) z += vertical;

    ptr_write_f32(player, OFF_PLAYER_X, x);
    ptr_write_f32(player, OFF_PLAYER_Y, y);
    ptr_write_f32(player, OFF_PLAYER_Z, z);
}

static void toggle_freeze_minigame_timers(void) {
    g_freeze_minigame_timers = !g_freeze_minigame_timers;
    write_log(g_freeze_minigame_timers ? "[cheat] minigame timer freeze enabled\r\n" : "[cheat] minigame timer freeze disabled\r\n");
}

static void win_struggle_now(void) {
    if (!g_offsets_ready) return;
    mem_write_i32(RVA_STRUGGLE_MY_ORBS, 200);
    mem_write_i32(RVA_STRUGGLE_FOE_ORBS, 0);
    write_log("[cheat] Struggle orbs set to 200 / 0\r\n");
}

static void grandstander_combo_now(void) {
    if (!g_offsets_ready) return;
    mem_write_i32(RVA_GRANDSTANDER_COMBO, 999);
    mem_write_i32(RVA_GRANDSTANDER_MAX, 999);
    write_log("[cheat] Grandstander combo set to 999\r\n");
}

static void restore_all_abilities(void) {
    BYTE* save;
    int i = 0;
    unsigned long long off;
    if (!g_abilities_original_valid) {
        g_all_abilities = FALSE;
        return;
    }
    save = global_ptr(RVA_SAVE_PTR);
    if (save && save == g_abilities_save_base) {
        for (off = 0x36C5ULL; off <= 0x3763ULL; off += 2ULL) {
            if (off == 0x3755ULL) continue;
            ptr_write_u8(save, off, g_abilities_original[i++]);
        }
    }
    g_all_abilities = FALSE;
    g_abilities_original_valid = FALSE;
    g_abilities_save_base = NULL;
}

static void toggle_all_abilities(void) {
    BYTE* save;
    int i = 0;
    unsigned long long off;
    if (g_all_abilities) {
        restore_all_abilities();
        write_log("[cheat] ability activation bits restored\r\n");
        return;
    }
    save = global_ptr(RVA_SAVE_PTR);
    if (!save) return;
    for (off = 0x36C5ULL; off <= 0x3763ULL; off += 2ULL) {
        if (off == 0x3755ULL) continue;
        g_abilities_original[i++] = ptr_read_u8(save, off);
    }
    g_abilities_save_base = save;
    g_abilities_original_valid = TRUE;
    g_all_abilities = TRUE;
    write_log("[cheat] all existing ability slots activated\r\n");
}

/* Resolve code patching APIs dynamically so the frozen import table stays unchanged. */
typedef BOOL (WINAPI *PFN_VIRTUALPROTECT)(LPVOID, SIZE_T, DWORD, DWORD*);
typedef LPVOID (WINAPI *PFN_VIRTUALALLOC)(LPVOID, SIZE_T, DWORD, DWORD);
typedef BOOL (WINAPI *PFN_FLUSHINSTRUCTIONCACHE)(HANDLE, LPCVOID, SIZE_T);

#define MEM_COMMIT_LOCAL             0x00001000UL
#define MEM_RESERVE_LOCAL            0x00002000UL
#define PAGE_EXECUTE_READWRITE_LOCAL 0x00000040UL

static PFN_VIRTUALPROTECT g_virtual_protect = NULL;
static PFN_VIRTUALALLOC g_virtual_alloc = NULL;
static PFN_FLUSHINSTRUCTIONCACHE g_flush_instruction_cache = NULL;

static BOOL resolve_patch_api(void) {
    HMODULE kernel;
    if (g_virtual_protect && g_virtual_alloc && g_flush_instruction_cache) return TRUE;
    kernel = GetModuleHandleW(L"KERNEL32.dll");
    if (!kernel) return FALSE;
    g_virtual_protect = (PFN_VIRTUALPROTECT)GetProcAddress(kernel, "VirtualProtect");
    g_virtual_alloc = (PFN_VIRTUALALLOC)GetProcAddress(kernel, "VirtualAlloc");
    g_flush_instruction_cache = (PFN_FLUSHINSTRUCTIONCACHE)GetProcAddress(kernel, "FlushInstructionCache");
    if (!g_virtual_protect || !g_virtual_alloc || !g_flush_instruction_cache) {
        g_virtual_protect = NULL;
        g_virtual_alloc = NULL;
        g_flush_instruction_cache = NULL;
        write_log("[hook] ERROR: could not resolve VirtualProtect/VirtualAlloc/FlushInstructionCache\r\n");
        return FALSE;
    }
    return TRUE;
}

static BOOL bytes_equal(const BYTE* a, const BYTE* b, unsigned int count) {
    unsigned int i;
    for (i = 0; i < count; ++i) if (a[i] != b[i]) return FALSE;
    return TRUE;
}

static void copy_bytes(BYTE* dst, const BYTE* src, unsigned int count) {
    unsigned int i;
    for (i = 0; i < count; ++i) dst[i] = src[i];
}

static void write_i32_le(BYTE* dst, int value) {
    unsigned int v = (unsigned int)value;
    dst[0] = (BYTE)(v & 0xFFU);
    dst[1] = (BYTE)((v >> 8) & 0xFFU);
    dst[2] = (BYTE)((v >> 16) & 0xFFU);
    dst[3] = (BYTE)((v >> 24) & 0xFFU);
}

static BOOL rel32_value(BYTE* instruction, unsigned int instruction_size, BYTE* destination, int* out_delta) {
    long long delta = (long long)(ULONG_PTR)destination - ((long long)(ULONG_PTR)instruction + (long long)instruction_size);
    if (delta < -2147483648LL || delta > 2147483647LL) return FALSE;
    *out_delta = (int)delta;
    return TRUE;
}

static BOOL write_code(BYTE* address, const BYTE* data, unsigned int count) {
    DWORD old_protect = 0;
    DWORD ignored = 0;
    if (!resolve_patch_api()) return FALSE;
    if (!g_virtual_protect(address, (SIZE_T)count, PAGE_EXECUTE_READWRITE_LOCAL, &old_protect)) {
        write_log("[hook] ERROR: VirtualProtect failed while enabling code patch\r\n");
        return FALSE;
    }
    copy_bytes(address, data, count);
    g_flush_instruction_cache((HANDLE)(long long)-1, address, (SIZE_T)count);
    g_virtual_protect(address, (SIZE_T)count, old_protect, &ignored);
    return TRUE;
}

static BYTE* allocate_ohk_cave(BYTE* target) {
    ULONG_PTR start;
    unsigned int i;
    if (g_ohk_cave) return g_ohk_cave;
    if (!resolve_patch_api()) return NULL;

    /* Start just after the verified KH2 image and search upward in 64 KiB steps. */
    start = ((ULONG_PTR)g_game_base + (ULONG_PTR)TARGET_IMAGE_SIZE + 0xFFFFULL) & ~0xFFFFULL;
    for (i = 0; i < 8192U; ++i) {
        ULONG_PTR candidate = start + ((ULONG_PTR)i << 16);
        long long delta = (long long)candidate - ((long long)(ULONG_PTR)target + 5LL);
        LPVOID p;
        if (delta > 0x70000000LL) break;
        p = g_virtual_alloc((LPVOID)candidate, (SIZE_T)0x1000, MEM_COMMIT_LOCAL | MEM_RESERVE_LOCAL, PAGE_EXECUTE_READWRITE_LOCAL);
        if (p) {
            g_ohk_cave = (BYTE*)p;
            write_log("[hook] One Hit Kill executable cave allocated near KH2\r\n");
            return g_ohk_cave;
        }
    }
    write_log("[hook] ERROR: could not allocate a near executable cave for One Hit Kill\r\n");
    return NULL;
}

static BOOL enable_one_hit_kill(void) {
    static const BYTE original[6] = {0x44,0x03,0xCA,0x44,0x3B,0xC8};
    BYTE* target;
    BYTE* cave;
    BYTE code[32] = {
        0x83,0xB9,0x70,0x02,0x00,0x00,0x00, /* cmp dword ptr [rcx+270],0 */
        0x74,0x0C,                           /* je original */
        0x41,0x81,0xC1,0x00,0x00,0x00,0x80, /* add r9d,80000000h */
        0x41,0x39,0xC1,                     /* cmp r9d,eax */
        0xEB,0x06,                           /* jmp return */
        0x41,0x01,0xD1,                     /* original: add r9d,edx */
        0x41,0x39,0xC1,                     /* original: cmp r9d,eax */
        0xE9,0x00,0x00,0x00,0x00            /* return to KH2 + 6 */
    };
    BYTE patch[6] = {0xE9,0,0,0,0,0x90};
    int delta;

    if (!g_offsets_ready || !g_game_base) return FALSE;
    target = g_game_base + RVA_CE_ONE_HIT_KILL;
    if (!bytes_equal(target, original, 6)) {
        write_log("[hook] BLOCKED One Hit Kill: original instruction bytes do not match this CE hook\r\n");
        return FALSE;
    }
    cave = allocate_ohk_cave(target);
    if (!cave) return FALSE;
    if (!rel32_value(cave + 27, 5, target + 6, &delta)) {
        write_log("[hook] ERROR: One Hit Kill cave return jump is out of range\r\n");
        return FALSE;
    }
    write_i32_le(code + 28, delta);
    copy_bytes(cave, code, sizeof(code));
    g_flush_instruction_cache((HANDLE)(long long)-1, cave, (SIZE_T)sizeof(code));

    if (!rel32_value(target, 5, cave, &delta)) {
        write_log("[hook] ERROR: One Hit Kill entry jump is out of range\r\n");
        return FALSE;
    }
    write_i32_le(patch + 1, delta);
    if (!write_code(target, patch, 6)) return FALSE;
    copy_bytes(g_ohk_patch_bytes, patch, 6);
    g_ohk_patch_bytes_valid = TRUE;
    g_one_hit_kill = TRUE;
    write_log("[hook] One Hit Kill enabled from Gear2 CE defense hook\r\n");
    return TRUE;
}

static void disable_one_hit_kill(void) {
    static const BYTE original[6] = {0x44,0x03,0xCA,0x44,0x3B,0xC8};
    BYTE* target;
    if (!g_one_hit_kill || !g_game_base) {
        g_one_hit_kill = FALSE;
        return;
    }
    target = g_game_base + RVA_CE_ONE_HIT_KILL;
    if (g_ohk_patch_bytes_valid && bytes_equal(target, g_ohk_patch_bytes, 6)) {
        if (write_code(target, original, 6)) write_log("[hook] One Hit Kill disabled and original bytes restored\r\n");
    } else {
        write_log("[hook] WARNING: One Hit Kill site changed externally; refusing to overwrite it during restore\r\n");
    }
    g_one_hit_kill = FALSE;
}

static void toggle_one_hit_kill(void) {
    if (g_one_hit_kill) disable_one_hit_kill();
    else enable_one_hit_kill();
}

static void restore_instant_haste(void) {
    BYTE* player;
    if (!g_instant_haste) return;
    player = global_ptr(RVA_PLAYER_PTR);
    if (player && g_haste_original_valid && player == g_haste_object) {
        ptr_write_f32(player, 0x20CULL, g_haste_original);
    }
    g_instant_haste = FALSE;
    g_haste_original_valid = FALSE;
    g_haste_object = NULL;
    write_log("[cheat] Instant Haste disabled and captured haste value restored\r\n");
}

static void toggle_instant_haste(void) {
    static const BYTE ce_haste_instruction[8] = {0xF3,0x0F,0x59,0x89,0x0C,0x02,0x00,0x00};
    if (g_instant_haste) {
        restore_instant_haste();
        return;
    }
    if (!g_offsets_ready || !g_game_base ||
        !bytes_equal(g_game_base + RVA_CE_INSTANT_HASTE, ce_haste_instruction, 8)) {
        write_log("[cheat] BLOCKED Instant Haste: Gear2 CE haste instruction did not match the running build\r\n");
        return;
    }
    g_haste_original_valid = FALSE;
    g_haste_object = NULL;
    g_instant_haste = TRUE;
    write_log("[cheat] Instant Haste enabled using verified CE haste field (100x recharge)\r\n");
}

static BOOL enable_free_command(void) {
    static const BYTE original[8] = {0x88,0x47,0x05,0x48,0x8B,0xCB,0x33,0xC0};
    static const BYTE patch[8]    = {0x33,0xC0,0x88,0x47,0x05,0x48,0x8B,0xCB};
    BYTE* target;
    if (!g_offsets_ready || !g_game_base) return FALSE;
    target = g_game_base + RVA_CE_FREE_COMMAND;
    if (!bytes_equal(target, original, 8)) {
        write_log("[hook] BLOCKED Free Command: original instruction bytes do not match this CE patch\r\n");
        return FALSE;
    }
    if (!write_code(target, patch, 8)) return FALSE;
    g_free_command = TRUE;
    write_log("[hook] Free Command enabled; command disabled byte is forced to zero\r\n");
    return TRUE;
}

static void disable_free_command(void) {
    static const BYTE original[8] = {0x88,0x47,0x05,0x48,0x8B,0xCB,0x33,0xC0};
    static const BYTE patch[8]    = {0x33,0xC0,0x88,0x47,0x05,0x48,0x8B,0xCB};
    BYTE* target;
    if (!g_free_command || !g_game_base) {
        g_free_command = FALSE;
        return;
    }
    target = g_game_base + RVA_CE_FREE_COMMAND;
    if (bytes_equal(target, patch, 8)) {
        if (write_code(target, original, 8)) write_log("[hook] Free Command disabled and original bytes restored\r\n");
    } else {
        write_log("[hook] WARNING: Free Command site changed externally; refusing to overwrite it during restore\r\n");
    }
    g_free_command = FALSE;
}

static void toggle_free_command(void) {
    if (g_free_command) disable_free_command();
    else enable_free_command();
}

static BOOL square_bit_is_main(int bit) {
    return bit == 0 || bit == 1 || bit == 3 || bit == 8 || bit == 10 ||
           bit == 14 || bit == 15 || bit == 16 || bit == 21;
}

static BOOL item_visible_in_view(int index) {
    if (index == ITEM_DISABLE_ALL) return TRUE;
    if (index >= ITEM_SQUARE_FIRST && index <= ITEM_SQUARE_LAST) {
        return g_advanced_view ? !square_bit_is_main(index - ITEM_SQUARE_FIRST)
                               : square_bit_is_main(index - ITEM_SQUARE_FIRST);
    }
    if (g_advanced_view) {
        return index == ITEM_UNLOCK_FORMS || index == ITEM_MAX_FORM_LEVELS ||
               index == ITEM_FREEZE_GAME_TIME || index == ITEM_WIN_STRUGGLE ||
               index == ITEM_GRANDSTANDER_COMBO || index == ITEM_ACTIVATE_ALL_ABILITIES;
    }
    return index != ITEM_UNLOCK_FORMS && index != ITEM_MAX_FORM_LEVELS &&
           index != ITEM_FREEZE_GAME_TIME && index != ITEM_WIN_STRUGGLE &&
           index != ITEM_GRANDSTANDER_COMBO && index != ITEM_ACTIVATE_ALL_ABILITIES;
}

static void ensure_selected_visible(void) {
    int i;
    if (g_selected >= 0 && g_selected < ITEM_COUNT && item_visible_in_view(g_selected)) return;
    for (i = 0; i < ITEM_COUNT; ++i) {
        if (item_visible_in_view(i)) {
            g_selected = i;
            return;
        }
    }
    g_selected = ITEM_DISABLE_ALL;
}

static void move_selection(int direction) {
    int i;
    int next = g_selected;
    for (i = 0; i < ITEM_COUNT; ++i) {
        next += direction;
        if (next < 0) next = ITEM_COUNT - 1;
        if (next >= ITEM_COUNT) next = 0;
        if (item_visible_in_view(next)) {
            g_selected = next;
            return;
        }
    }
}

static void disable_and_restore_all(void) {
    disable_one_hit_kill();
    restore_instant_haste();
    disable_free_command();
    g_inf_hp = FALSE;
    g_inf_mp = FALSE;
    g_inf_drive = FALSE;
    g_inf_drive_form = FALSE;
    g_freeze_game_time = FALSE;
    g_freeze_minigame_timers = FALSE;
    g_exp_multiplier = 1;
    g_exp_prev_valid = FALSE;
    g_munny_multiplier = 1;
    g_munny_prev_valid = FALSE;
    g_fly_mode = FALSE;
    g_low_gravity = FALSE;
    restore_weight_if_possible();
    restore_turbo_movement();
    restore_all_abilities();
    restore_speed();
    restore_munny();
    restore_drive_max();
    restore_forms();
    restore_form_levels();
    restore_square_flags();
    write_log("[cheat] all cheats disabled and reversible state restored; already-earned EXP/munny is not rolled back\r\n");
}

static void set_speed_mode(int mode) {
    if (!g_offsets_ready) return;
    if (g_speed_mode == mode) {
        restore_speed();
        write_log("[cheat] game speed restored\r\n");
        return;
    }
    if (!g_speed_original_valid) {
        float current = mem_read_f32(RVA_GAME_SPEED);
        if (current < 0.05f || current > 10.0f) current = 1.0f;
        g_speed_original = current;
        g_speed_original_valid = TRUE;
    }
    g_speed_mode = mode;
    if (mode == 1) write_log("[cheat] 0.5x game speed enabled\r\n");
    else if (mode == 2) write_log("[cheat] 2x game speed enabled\r\n");
    else if (mode == 3) write_log("[cheat] 4x game speed enabled\r\n");
}

static void toggle_munny_lock(void) {
    if (!g_offsets_ready || !player_stats_valid()) return;
    if (g_munny_lock) {
        restore_munny();
        write_log("[cheat] munny lock disabled and original restored\r\n");
        return;
    }
    {
        int current = mem_read_i32(RVA_SAVE + OFF_MUNNY);
        if (current < 0 || current > 99999999) return;
        g_munny_original = current;
        g_munny_original_valid = TRUE;
        g_munny_lock = TRUE;
        write_log("[cheat] munny lock enabled\r\n");
    }
}

static void toggle_drive_max9(void) {
    if (!g_offsets_ready || !player_stats_valid()) return;
    if (g_drive_max9) {
        restore_drive_max();
        write_log("[cheat] max drive 9 disabled and original restored\r\n");
        return;
    }
    g_drive_percent_original = mem_read_u8(RVA_SLOT1 + OFF_DRIVE_PERCENT);
    g_drive_current_original = mem_read_u8(RVA_SLOT1 + OFF_DRIVE_CURRENT);
    g_drive_max_original = mem_read_u8(RVA_SLOT1 + OFF_DRIVE_MAX);
    g_drive_original_valid = TRUE;
    g_drive_max9 = TRUE;
    write_log("[cheat] temporary max drive 9 enabled\r\n");
}

static void toggle_unlock_forms(void) {
    if (!g_offsets_ready || !player_stats_valid()) return;
    if (g_unlock_forms) {
        restore_forms();
        write_log("[cheat] standard drive forms restored\r\n");
        return;
    }
    g_forms_original = mem_read_u8(RVA_SAVE + OFF_FORM_UNLOCKS);
    g_forms_original_valid = TRUE;
    g_unlock_forms = TRUE;
    write_log("[cheat] temporary standard drive forms enabled\r\n");
}

static void toggle_max_form_levels(void) {
    int i;
    if (!g_offsets_ready || !player_stats_valid()) return;
    if (g_max_form_levels) {
        restore_form_levels();
        write_log("[cheat] form levels restored\r\n");
        return;
    }
    for (i = 0; i < 5; ++i) {
        BYTE level = mem_read_u8(RVA_SAVE + OFF_FORM_LEVEL + FORM_LEVEL_STRIDE * (unsigned long long)i);
        if (level > 7) return;
        g_form_levels_original[i] = level;
    }
    g_form_levels_original_valid = TRUE;
    g_max_form_levels = TRUE;
    write_log("[cheat] temporary max form levels enabled\r\n");
}

static void full_refill_now(void) {
    if (!player_stats_valid()) return;
    {
        int max_hp = mem_read_i32(RVA_SLOT1 + OFF_HP_MAX);
        int max_mp = mem_read_i32(RVA_SLOT1 + OFF_MP_MAX);
        BYTE max_drive = mem_read_u8(RVA_SLOT1 + OFF_DRIVE_MAX);
        mem_write_i32(RVA_SLOT1 + OFF_HP_CURRENT, max_hp);
        mem_write_i32(RVA_SLOT1 + OFF_MP_CURRENT, max_mp);
        mem_write_u8(RVA_SLOT1 + OFF_DRIVE_CURRENT, max_drive);
        mem_write_u8(RVA_SLOT1 + OFF_DRIVE_PERCENT, 100);
        write_log("[cheat] full refill action executed\r\n");
    }
}

static void activate_selected(void) {
    if (!g_offsets_ready && g_selected != ITEM_DISABLE_ALL) return;

    if (g_selected >= ITEM_SQUARE_FIRST && g_selected <= ITEM_SQUARE_LAST) {
        toggle_square_flag(g_selected - ITEM_SQUARE_FIRST);
        if (g_overlay_window) InvalidateRect(g_overlay_window, NULL, TRUE);
        return;
    }

    switch (g_selected) {
        case ITEM_INF_HP:
            g_inf_hp = !g_inf_hp;
            write_log(g_inf_hp ? "[cheat] infinite HP enabled\r\n" : "[cheat] infinite HP disabled\r\n");
            break;
        case ITEM_INF_MP:
            g_inf_mp = !g_inf_mp;
            write_log(g_inf_mp ? "[cheat] infinite MP enabled\r\n" : "[cheat] infinite MP disabled\r\n");
            break;
        case ITEM_INF_DRIVE:
            g_inf_drive = !g_inf_drive;
            write_log(g_inf_drive ? "[cheat] infinite drive enabled\r\n" : "[cheat] infinite drive disabled\r\n");
            break;
        case ITEM_SPEED_HALF: set_speed_mode(1); break;
        case ITEM_SPEED_DOUBLE: set_speed_mode(2); break;
        case ITEM_SPEED_QUAD: set_speed_mode(3); break;
        case ITEM_MUNNY_LOCK: toggle_munny_lock(); break;
        case ITEM_DRIVE_MAX9: toggle_drive_max9(); break;
        case ITEM_UNLOCK_FORMS: toggle_unlock_forms(); break;
        case ITEM_MAX_FORM_LEVELS: toggle_max_form_levels(); break;
        case ITEM_REFILL_NOW: full_refill_now(); break;
        case ITEM_FREEZE_GAME_TIME: toggle_freeze_game_time(); break;
        case ITEM_INF_DRIVE_FORM: toggle_inf_drive_form(); break;
        case ITEM_EXP_MULTIPLIER: cycle_exp_multiplier(); break;
        case ITEM_MUNNY_MULTIPLIER: cycle_munny_multiplier(); break;
        case ITEM_TURBO_MOVEMENT: toggle_turbo_movement(); break;
        case ITEM_FLY_MODE: toggle_fly_mode(); break;
        case ITEM_LOW_GRAVITY: toggle_low_gravity(); break;
        case ITEM_FREEZE_MINIGAME_TIMERS: toggle_freeze_minigame_timers(); break;
        case ITEM_WIN_STRUGGLE: win_struggle_now(); break;
        case ITEM_GRANDSTANDER_COMBO: grandstander_combo_now(); break;
        case ITEM_ACTIVATE_ALL_ABILITIES: toggle_all_abilities(); break;
        case ITEM_ONE_HIT_KILL: toggle_one_hit_kill(); break;
        case ITEM_INSTANT_HASTE: toggle_instant_haste(); break;
        case ITEM_FREE_COMMAND: toggle_free_command(); break;
        case ITEM_DISABLE_ALL: disable_and_restore_all(); break;
        default: break;
    }
    if (g_overlay_window) InvalidateRect(g_overlay_window, NULL, TRUE);
}

static void apply_cheats(void) {
    if (!g_offsets_ready || !g_game_base) return;

    if (player_stats_valid()) {
        if (g_inf_hp) {
            int max_hp = mem_read_i32(RVA_SLOT1 + OFF_HP_MAX);
            mem_write_i32(RVA_SLOT1 + OFF_HP_CURRENT, max_hp);
        }
        if (g_inf_mp) {
            int max_mp = mem_read_i32(RVA_SLOT1 + OFF_MP_MAX);
            mem_write_i32(RVA_SLOT1 + OFF_MP_CURRENT, max_mp);
        }
        if (g_inf_drive) {
            BYTE max_drive = mem_read_u8(RVA_SLOT1 + OFF_DRIVE_MAX);
            mem_write_u8(RVA_SLOT1 + OFF_DRIVE_CURRENT, max_drive);
            mem_write_u8(RVA_SLOT1 + OFF_DRIVE_PERCENT, 100);
        }
        if (g_drive_max9) {
            mem_write_u8(RVA_SLOT1 + OFF_DRIVE_MAX, 9);
            mem_write_u8(RVA_SLOT1 + OFF_DRIVE_CURRENT, 9);
            mem_write_u8(RVA_SLOT1 + OFF_DRIVE_PERCENT, 100);
        }
    }

    if (g_speed_mode == 1) mem_write_f32(RVA_GAME_SPEED, 0.5f);
    else if (g_speed_mode == 2) mem_write_f32(RVA_GAME_SPEED, 2.0f);
    else if (g_speed_mode == 3) mem_write_f32(RVA_GAME_SPEED, 4.0f);

    if (g_munny_lock) mem_write_i32(RVA_SAVE + OFF_MUNNY, 999999);

    if (g_freeze_game_time) mem_write_u32(RVA_GAME_TIME, g_frozen_game_time);

    if (g_unlock_forms) {
        BYTE current = mem_read_u8(RVA_SAVE + OFF_FORM_UNLOCKS);
        mem_write_u8(RVA_SAVE + OFF_FORM_UNLOCKS, (BYTE)(current | 0x56));
    }

    if (g_max_form_levels) {
        int i;
        for (i = 0; i < 5; ++i) {
            mem_write_u8(RVA_SAVE + OFF_FORM_LEVEL + FORM_LEVEL_STRIDE * (unsigned long long)i, 7);
        }
    }

    if (g_inf_drive_form) {
        BYTE* player = global_ptr(RVA_PLAYER_PTR);
        if (player) {
            float max_time = ptr_read_f32(player, OFF_DRIVE_FORM_MAX);
            if (float_sane(max_time, 0.0f, 100000.0f) && max_time > 0.0f) {
                ptr_write_f32(player, OFF_DRIVE_FORM_CURRENT, max_time);
            }
        }
    }

    if (g_instant_haste) {
        BYTE* player = global_ptr(RVA_PLAYER_PTR);
        if (player) {
            float current = ptr_read_f32(player, 0x20CULL);
            if (!g_haste_original_valid || player != g_haste_object) {
                if (float_sane(current, -1000.0f, 1000.0f)) {
                    g_haste_object = player;
                    g_haste_original = current;
                    g_haste_original_valid = TRUE;
                }
            }
            if (g_haste_original_valid && player == g_haste_object) ptr_write_f32(player, 0x20CULL, 100.0f);
        }
    }

    if (g_exp_multiplier > 1) {
        BYTE* save = global_ptr(RVA_SAVE_PTR);
        if (save) {
            int current = ptr_read_i32(save, OFF_CE_EXP);
            if (current >= 0 && current <= 99999999) {
                if (!g_exp_prev_valid) {
                    g_exp_prev = current;
                    g_exp_prev_valid = TRUE;
                } else if (current > g_exp_prev) {
                    long long delta = (long long)current - (long long)g_exp_prev;
                    long long boosted = (long long)g_exp_prev + delta * (long long)g_exp_multiplier;
                    if (boosted > 99999999LL) boosted = 99999999LL;
                    ptr_write_i32(save, OFF_CE_EXP, (int)boosted);
                    g_exp_prev = (int)boosted;
                } else {
                    g_exp_prev = current;
                }
            } else {
                g_exp_prev_valid = FALSE;
            }
        }
    }

    if (g_munny_multiplier > 1) {
        int current = mem_read_i32(RVA_SAVE + OFF_MUNNY);
        if (current >= 0 && current <= 99999999) {
            if (!g_munny_prev_valid) {
                g_munny_prev = current;
                g_munny_prev_valid = TRUE;
            } else if (current > g_munny_prev) {
                long long delta = (long long)current - (long long)g_munny_prev;
                long long boosted = (long long)g_munny_prev + delta * (long long)g_munny_multiplier;
                if (boosted > 99999999LL) boosted = 99999999LL;
                mem_write_i32(RVA_SAVE + OFF_MUNNY, (int)boosted);
                g_munny_prev = (int)boosted;
            } else {
                g_munny_prev = current;
            }
        } else {
            g_munny_prev_valid = FALSE;
        }
    }

    if (g_turbo_movement) {
        BYTE* player = global_ptr(RVA_MOTION_PTR);
        int form = movement_form();
        if (player) {
            switch (form) {
                case 0: ptr_write_f32(player, OFF_PLAYER_SPEED, 15.0f); ptr_write_f32(player, OFF_PLAYER_JUMP, 600.0f); ptr_write_f32(player, OFF_PLAYER_TURN, 0.26f); break;
                case 1: ptr_write_f32(player, OFF_PLAYER_SPEED, 18.0f); ptr_write_f32(player, OFF_PLAYER_JUMP, 800.0f); ptr_write_f32(player, OFF_PLAYER_TURN, 0.32f); break;
                case 2: ptr_write_f32(player, OFF_PLAYER_SPEED, 24.0f); ptr_write_f32(player, OFF_PLAYER_JUMP, 600.0f); ptr_write_f32(player, OFF_PLAYER_TURN, 0.60f); break;
                case 3: ptr_write_f32(player, OFF_PLAYER_SPEED, 15.0f); ptr_write_f32(player, OFF_PLAYER_JUMP, 600.0f); ptr_write_f32(player, OFF_PLAYER_TURN, 0.26f); break;
                case 4: ptr_write_f32(player, OFF_PLAYER_SPEED, 20.0f); ptr_write_f32(player, OFF_PLAYER_JUMP, 900.0f); ptr_write_f32(player, OFF_PLAYER_TURN, 0.45f); break;
                case 5: ptr_write_f32(player, OFF_PLAYER_SPEED, 30.0f); ptr_write_f32(player, OFF_PLAYER_JUMP, 1200.0f); ptr_write_f32(player, OFF_PLAYER_TURN, 0.65f); break;
                case 6: ptr_write_f32(player, OFF_PLAYER_SPEED, 40.0f); ptr_write_f32(player, OFF_PLAYER_JUMP, 1800.0f); ptr_write_f32(player, OFF_PLAYER_TURN, 0.80f); break;
                default: break;
            }
        }
    }

    if (g_fly_mode) {
        apply_fly_mode();
    } else if (g_low_gravity) {
        BYTE* player = global_ptr(RVA_MOTION_PTR);
        if (player) {
            capture_weight_if_needed(player);
            ptr_write_f32(player, OFF_PLAYER_WEIGHT, 4.0f);
        }
    }

    if (g_freeze_minigame_timers) {
        mem_write_u32(RVA_MINIGAME_TIME, 0);
        mem_write_u32(RVA_MINIGAME_TIME_START, 0);
    }

    if (g_all_abilities) {
        BYTE* save = global_ptr(RVA_SAVE_PTR);
        if (!save || save != g_abilities_save_base) {
            g_all_abilities = FALSE;
            g_abilities_original_valid = FALSE;
            g_abilities_save_base = NULL;
            write_log("[cheat] ability activation disabled because save pointer changed\r\n");
        } else {
            unsigned long long off;
            for (off = 0x36C5ULL; off <= 0x3763ULL; off += 2ULL) {
                BYTE value;
                if (off == 0x3755ULL) continue;
                value = ptr_read_u8(save, off);
                ptr_write_u8(save, off, (BYTE)(value | 0x80));
            }
        }
    }
}

static BOOL CALLBACK find_game_window_cb(HWND hwnd, LPARAM lparam) {
    (void)lparam;
    if (!IsWindowVisible(hwnd)) return TRUE;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != GetCurrentProcessId()) return TRUE;
    RECT r;
    if (!GetWindowRect(hwnd, &r)) return TRUE;
    LONG w = r.right - r.left;
    LONG h = r.bottom - r.top;
    if (w < 640 || h < 360) return TRUE;
    g_game_window = hwnd;
    return FALSE;
}

static HWND find_game_window(void) {
    g_game_window = NULL;
    EnumWindows(find_game_window_cb, 0);
    return g_game_window;
}

static void center_overlay(void) {
    if (!g_game_window || !g_overlay_window) return;
    RECT r;
    if (!GetWindowRect(g_game_window, &r)) return;
    int x = r.left + ((r.right - r.left) - MENU_W) / 2;
    int y = r.top + ((r.bottom - r.top) - MENU_H) / 2;
    SetWindowPos(g_overlay_window, HWND_TOPMOST, x, y, MENU_W, MENU_H, SWP_NOACTIVATE | (g_visible ? SWP_SHOWWINDOW : 0));
}

static void draw_line(HDC dc, int x, int y, int w, int h, const wchar_t* text, COLORREF color) {
    RECT r;
    r.left = x; r.top = y; r.right = x + w; r.bottom = y + h;
    SetTextColor(dc, color);
    DrawTextW(dc, text, -1, &r, DT_LEFT | DT_TOP | DT_NOPREFIX | DT_WORDBREAK);
}

static BOOL item_enabled(int index) {
    if (index >= ITEM_SQUARE_FIRST && index <= ITEM_SQUARE_LAST) {
        return square_flag_enabled(index - ITEM_SQUARE_FIRST);
    }

    switch (index) {
        case ITEM_INF_HP: return g_inf_hp;
        case ITEM_INF_MP: return g_inf_mp;
        case ITEM_INF_DRIVE: return g_inf_drive;
        case ITEM_SPEED_HALF: return g_speed_mode == 1;
        case ITEM_SPEED_DOUBLE: return g_speed_mode == 2;
        case ITEM_SPEED_QUAD: return g_speed_mode == 3;
        case ITEM_MUNNY_LOCK: return g_munny_lock;
        case ITEM_DRIVE_MAX9: return g_drive_max9;
        case ITEM_UNLOCK_FORMS: return g_unlock_forms;
        case ITEM_MAX_FORM_LEVELS: return g_max_form_levels;
        case ITEM_FREEZE_GAME_TIME: return g_freeze_game_time;
        case ITEM_INF_DRIVE_FORM: return g_inf_drive_form;
        case ITEM_EXP_MULTIPLIER: return g_exp_multiplier > 1;
        case ITEM_MUNNY_MULTIPLIER: return g_munny_multiplier > 1;
        case ITEM_TURBO_MOVEMENT: return g_turbo_movement;
        case ITEM_FLY_MODE: return g_fly_mode;
        case ITEM_LOW_GRAVITY: return g_low_gravity;
        case ITEM_FREEZE_MINIGAME_TIMERS: return g_freeze_minigame_timers;
        case ITEM_ACTIVATE_ALL_ABILITIES: return g_all_abilities;
        case ITEM_ONE_HIT_KILL: return g_one_hit_kill;
        case ITEM_INSTANT_HASTE: return g_instant_haste;
        case ITEM_FREE_COMMAND: return g_free_command;
        default: return FALSE;
    }
}

static const wchar_t* item_label(int index) {
    if (index >= ITEM_SQUARE_FIRST && index <= ITEM_SQUARE_LAST) {
        return g_square_flag_labels[index - ITEM_SQUARE_FIRST];
    }

    switch (index) {
        case ITEM_INF_HP: return L"Infinite HP";
        case ITEM_INF_MP: return L"Infinite MP";
        case ITEM_INF_DRIVE: return L"Infinite Drive Gauge";
        case ITEM_SPEED_HALF: return L"Game Speed 0.5x";
        case ITEM_SPEED_DOUBLE: return L"Game Speed 2x";
        case ITEM_SPEED_QUAD: return L"Game Speed 4x";
        case ITEM_MUNNY_LOCK: return L"Lock Munny at 999999";
        case ITEM_DRIVE_MAX9: return L"Max Drive Gauge 9  TEMP";
        case ITEM_UNLOCK_FORMS: return L"Unlock Standard Drive Forms  TEMP EXP";
        case ITEM_MAX_FORM_LEVELS: return L"Max Form Levels 7  TEMP EXP";
        case ITEM_REFILL_NOW: return L"Full Refill Now";
        case ITEM_FREEZE_GAME_TIME: return L"Freeze Total Game Time  CT";
        case ITEM_INF_DRIVE_FORM: return L"Infinite Drive Form Duration  CE";
        case ITEM_EXP_MULTIPLIER: return L"EXP Gain Multiplier  CE SAVE";
        case ITEM_MUNNY_MULTIPLIER: return L"Munny Gain Multiplier  CE SAVE";
        case ITEM_TURBO_MOVEMENT: return L"Luvvy Turbo Movement  CE";
        case ITEM_FLY_MODE: return L"Fly Mode  WASD + Space/Ctrl  Shift=fast";
        case ITEM_LOW_GRAVITY: return L"Low Gravity";
        case ITEM_FREEZE_MINIGAME_TIMERS: return L"Freeze All Minigame Timers  CE";
        case ITEM_WIN_STRUGGLE: return L"Win Struggle Orbs Now  CE";
        case ITEM_GRANDSTANDER_COMBO: return L"Grandstander Combo 999 Now  CE";
        case ITEM_ACTIVATE_ALL_ABILITIES: return L"Activate All Existing Ability Slots  CE";
        case ITEM_ONE_HIT_KILL: return L"One Hit Kill  CE HOOK";
        case ITEM_INSTANT_HASTE: return L"Instant Haste  100x MP Recharge  CE";
        case ITEM_FREE_COMMAND: return L"Free Command  Commands Never Greyed  CE PATCH";
        case ITEM_DISABLE_ALL: return L"Disable + Restore All";
        default: return L"Unknown";
    }
}

static BOOL item_is_action(int index) {
    return index == ITEM_REFILL_NOW || index == ITEM_WIN_STRUGGLE ||
           index == ITEM_GRANDSTANDER_COMBO || index == ITEM_DISABLE_ALL;
}

static BOOL item_is_dangerous_square(int index) {
    int bit;
    if (index < ITEM_SQUARE_FIRST || index > ITEM_SQUARE_LAST) return FALSE;
    bit = index - ITEM_SQUARE_FIRST;
    return bit == 6 || bit == 13 || bit == 22 || bit == 23;
}

static void draw_item(HDC dc, int index, int y) {
    wchar_t line[192];
    w_clear(line);
    w_append(line, index == g_selected ? L"> " : L"  ", 192);
    if (item_is_action(index)) w_append(line, L"[RUN] ", 192);
    else w_append(line, item_enabled(index) ? L"[ON ] " : L"[OFF] ", 192);
    w_append(line, item_label(index), 192);
    if (index == ITEM_EXP_MULTIPLIER) {
        w_append(line, L"  [", 192); w_append_u32(line, (unsigned int)g_exp_multiplier, 192); w_append(line, L"x]", 192);
    } else if (index == ITEM_MUNNY_MULTIPLIER) {
        w_append(line, L"  [", 192); w_append_u32(line, (unsigned int)g_munny_multiplier, 192); w_append(line, L"x]", 192);
    }

    COLORREF color;
    if (index == g_selected) color = RGB(245, 225, 130);
    else if (item_is_dangerous_square(index)) color = RGB(235, 130, 130);
    else if (index == ITEM_UNLOCK_FORMS || index == ITEM_MAX_FORM_LEVELS) color = RGB(225, 185, 135);
    else if (item_enabled(index)) color = RGB(145, 220, 165);
    else color = RGB(220, 220, 225);
    draw_line(dc, 30, y, 660, 26, line, color);
}

static void draw_live_status(HDC dc) {
    wchar_t line[320];
    w_clear(line);
    if (!g_offsets_ready) {
        w_append(line, L"Offsets: BLOCKED. Exact Steam 1.0.0.2 build was not verified.", 320);
        draw_line(dc, 28, 82, 664, 25, line, RGB(235, 130, 130));
        return;
    }

    w_append(line, L"Offsets: VERIFIED Steam 1.0.0.2", 320);
    draw_line(dc, 28, 82, 664, 25, line, RGB(145, 220, 165));

    w_clear(line);
    if (player_stats_valid()) {
        int hp = mem_read_i32(RVA_SLOT1 + OFF_HP_CURRENT);
        int hpmax = mem_read_i32(RVA_SLOT1 + OFF_HP_MAX);
        int mp = mem_read_i32(RVA_SLOT1 + OFF_MP_CURRENT);
        int mpmax = mem_read_i32(RVA_SLOT1 + OFF_MP_MAX);
        BYTE drive = mem_read_u8(RVA_SLOT1 + OFF_DRIVE_CURRENT);
        BYTE drivemax = mem_read_u8(RVA_SLOT1 + OFF_DRIVE_MAX);
        int munny = mem_read_i32(RVA_SAVE + OFF_MUNNY);
        BYTE world = mem_read_u8(RVA_NOW + 0);
        BYTE room = mem_read_u8(RVA_NOW + 1);

        w_append(line, L"Live  HP ", 320); w_append_int_nonnegative(line, hp, 320);
        w_append(line, L"/", 320); w_append_int_nonnegative(line, hpmax, 320);
        w_append(line, L"   MP ", 320); w_append_int_nonnegative(line, mp, 320);
        w_append(line, L"/", 320); w_append_int_nonnegative(line, mpmax, 320);
        w_append(line, L"   Drive ", 320); w_append_u32(line, drive, 320);
        w_append(line, L"/", 320); w_append_u32(line, drivemax, 320);
        w_append(line, L"   Munny ", 320); w_append_int_nonnegative(line, munny, 320);
        w_append(line, L"   W", 320); w_append_u32(line, world, 320);
        w_append(line, L" R", 320); w_append_u32(line, room, 320);
    } else {
        w_append(line, L"Live player stats unavailable on this screen. Cheats wait for a valid Sora slot.", 320);
    }
    draw_line(dc, 28, 108, 664, 30, line, RGB(190, 195, 205));
}

static LRESULT CALLBACK overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    (void)wparam; (void)lparam;
    if (msg == WM_NCHITTEST) return HTTRANSPARENT;
    if (msg == WM_ERASEBKGND) return 1;
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT bg; bg.left = 0; bg.top = 0; bg.right = MENU_W; bg.bottom = MENU_H;
        HBRUSH brush = CreateSolidBrush(RGB(18, 20, 26));
        FillRect(dc, &bg, brush);
        DeleteObject(brush);
        SetBkMode(dc, TRANSPARENT);

        draw_line(dc, 28, 18, 664, 34, L"Luvvy KH2 Mod Menu", RGB(235, 238, 245));
        draw_line(dc, 28, 51, 664, 28, L"v0.5.0   F10 close   F9 Main/Advanced   F6/F7 select   F8 toggle/run", RGB(155, 175, 215));
        draw_live_status(dc);

        {
            int visible[ITEM_COUNT];
            int count = 0;
            int selected_pos = 0;
            int pos;
            int first;
            int last;
            int y = 150;
            for (pos = 0; pos < ITEM_COUNT; ++pos) {
                if (item_visible_in_view(pos)) {
                    if (pos == g_selected) selected_pos = count;
                    visible[count++] = pos;
                }
            }
            first = selected_pos - (VISIBLE_ITEMS / 2);
            if (first < 0) first = 0;
            if (first > count - VISIBLE_ITEMS) first = count - VISIBLE_ITEMS;
            if (first < 0) first = 0;
            last = first + VISIBLE_ITEMS;
            if (last > count) last = count;
            for (pos = first; pos < last; ++pos) {
                draw_item(dc, visible[pos], y);
                y += 31;
            }
        }

        draw_line(dc, 28, 526, 664, 28,
                  g_advanced_view ? L"ADVANCED PAGE: obscure Square flags + experimental/progression/minigame tools. F9 returns to Main."
                                  : L"MAIN PAGE: curated combat, movement, fun and useful Square developer cheats. F9 opens Advanced.",
                  g_advanced_view ? RGB(225, 185, 135) : RGB(145, 220, 165));
        draw_line(dc, 28, 556, 664, 36,
                  g_advanced_view ? L"Red Square flags are intentionally hidden here because they may crash, break loading, or trigger debug behavior."
                                  : L"CE hooks validate their original instruction bytes before patching. Fly: WASD, Space/Ctrl, Shift=fast.",
                  g_advanced_view ? RGB(235, 130, 130) : RGB(190, 195, 205));
        draw_line(dc, 28, 594, 664, 38,
                  L"END restores captured reversible state. EXP/Munny gain multipliers do not undo gains already written to the save.",
                  RGB(175, 180, 192));

        EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg == WM_CLOSE) {
        ShowWindow(hwnd, SW_HIDE);
        g_visible = FALSE;
        return 0;
    }
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static BOOL create_overlay(void) {
    WNDCLASSEXW wc;
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = 0;
    wc.lpfnWndProc = overlay_wndproc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_self;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"LuvvyKH2OverlayClass";
    wc.hIconSm = NULL;
    if (!RegisterClassExW(&wc)) {
        write_log("[menu] ERROR: RegisterClassExW failed\r\n");
        return FALSE;
    }

    DWORD ex = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
    g_overlay_window = CreateWindowExW(ex, L"LuvvyKH2OverlayClass", L"Luvvy KH2 Mod Menu", WS_POPUP,
                                      100, 100, MENU_W, MENU_H, NULL, NULL, g_self, NULL);
    if (!g_overlay_window) {
        write_log("[menu] ERROR: CreateWindowExW failed\r\n");
        return FALSE;
    }
    SetLayeredWindowAttributes(g_overlay_window, 0, 242, LWA_ALPHA);
    center_overlay();
    ShowWindow(g_overlay_window, SW_HIDE);
    UpdateWindow(g_overlay_window);
    write_log("[menu] overlay created\r\n");
    return TRUE;
}

static BOOL edge_key(int vk, BOOL* previous) {
    BOOL down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    BOOL pressed = down && !(*previous);
    *previous = down;
    return pressed;
}

static BOOL key_group_edge(int vk1, int vk2, BOOL* previous) {
    BOOL down1 = (GetAsyncKeyState(vk1) & 0x8000) != 0;
    BOOL down2 = (GetAsyncKeyState(vk2) & 0x8000) != 0;
    BOOL down = down1 || down2;
    BOOL pressed = down && !(*previous);
    *previous = down;
    return pressed;
}

static void handle_hotkeys(void) {
    if (edge_key(VK_F10, &g_f10_down)) {
        g_visible = !g_visible;
        if (g_visible) {
            center_overlay();
            ShowWindow(g_overlay_window, SW_SHOWNOACTIVATE);
            InvalidateRect(g_overlay_window, NULL, TRUE);
            write_log("[menu] F10: visible\r\n");
        } else {
            ShowWindow(g_overlay_window, SW_HIDE);
            write_log("[menu] F10: hidden\r\n");
        }
    }

    if (g_visible) {
        if (edge_key(VK_F9, &g_f9_down)) {
            g_advanced_view = !g_advanced_view;
            ensure_selected_visible();
            InvalidateRect(g_overlay_window, NULL, TRUE);
            write_log(g_advanced_view ? "[menu] advanced page visible\r\n" : "[menu] main page visible\r\n");
        }
        if (key_group_edge(VK_F6, VK_UP, &g_prev_down)) {
            move_selection(-1);
            InvalidateRect(g_overlay_window, NULL, TRUE);
        }
        if (key_group_edge(VK_F7, VK_DOWN, &g_next_down)) {
            move_selection(1);
            InvalidateRect(g_overlay_window, NULL, TRUE);
        }
        if (key_group_edge(VK_F8, VK_RETURN, &g_action_down)) {
            activate_selected();
        }
    } else {
        g_f9_down = FALSE;
        g_prev_down = FALSE;
        g_next_down = FALSE;
        g_action_down = FALSE;
    }

    if (edge_key(VK_END, &g_end_down)) {
        g_running = FALSE;
    }
}

static DWORD WINAPI menu_worker(LPVOID unused) {
    (void)unused;
    Sleep(100);
    write_log("[menu] KH2ModMenu.dll loaded v0.5.0\r\n");
    write_log("[menu] waiting for KH2 game window\r\n");

    while (g_running && !find_game_window()) Sleep(250);
    if (!g_running) FreeLibraryAndExitThread(g_self, 0);
    write_log("[menu] game window found\r\n");

    g_offsets_ready = verify_target_build();
    if (g_offsets_ready) {
        g_logged_verified = TRUE;
        write_log("[core] exact Steam 1.0.0.2 target verified; cheat writes enabled\r\n");
    } else {
        write_log("[core] target build NOT verified; cheat writes blocked\r\n");
    }

    if (!create_overlay()) {
        write_log("[menu] initialization failed\r\n");
        FreeLibraryAndExitThread(g_self, 1);
    }

    unsigned int reposition_counter = 0;
    unsigned int repaint_counter = 0;
    while (g_running) {
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (!g_offsets_ready) {
            g_offsets_ready = verify_target_build();
            if (g_offsets_ready && !g_logged_verified) {
                g_logged_verified = TRUE;
                write_log("[core] exact Steam 1.0.0.2 target verified; cheat writes enabled\r\n");
            }
        }

        handle_hotkeys();
        apply_cheats();

        if (g_visible) {
            if ((++reposition_counter % 100) == 0) center_overlay();
            if ((++repaint_counter % 10) == 0) InvalidateRect(g_overlay_window, NULL, TRUE);
        }
        Sleep(10);
    }

    write_log("[menu] restoring reversible cheats before unload\r\n");
    disable_and_restore_all();
    write_log("[menu] unloading\r\n");
    if (g_overlay_window) DestroyWindow(g_overlay_window);
    g_overlay_window = NULL;
    FreeLibraryAndExitThread(g_self, 0);
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = instance;
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(NULL, 0, menu_worker, NULL, 0, NULL);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
