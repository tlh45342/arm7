#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "arm7vm/arm7vm.h"
#include "default_bios.h"

#define APP_NAME        "arm7-runx"
#define APP_VERSION     "0.0.7"

#define DEFAULT_RAM     (256u * 1024u * 1024u)
#define LOAD_ADDRESS    0x00008000u
#define BIOS_ADDRESS    0x00000000u
#define BOOT_ADDRESS    0x00010000u
#define MONITOR_ADDRESS 0x00020000u

#define RUN_TIMER_ID    1u
#define RUN_TIMER_MS    10u
#define RUN_SLICE       2000u

#define CRT_BASE        0x0A000000u
#define CRT_COLS        80
#define CRT_ROWS        25
#define CRT_CELL_BYTES  2

#define IDM_MACHINE_RESET    1001
#define IDM_MACHINE_STEP     1002
#define IDM_MACHINE_RUN100   1003
#define IDM_MACHINE_RUN1000  1004
#define IDM_MACHINE_RUN10000 1005
#define IDM_MACHINE_EXIT     1006
#define IDM_MACHINE_START    1007
#define IDM_MACHINE_STOP     1008
#define IDM_MACHINE_START_BIOS    1010
#define IDM_MACHINE_START_FLAT    1011
#define IDM_MACHINE_START_BOOT    1012
#define IDM_MACHINE_START_MONITOR 1013

#define IDM_FIRMWARE_LOAD_FLAT    1101
#define IDM_FIRMWARE_LOAD_BIOS    1102
#define IDM_FIRMWARE_LOAD_BOOT    1103
#define IDM_FIRMWARE_LOAD_MONITOR 1104
#define IDM_FIRMWARE_EMBEDDED_BIOS 1105

#define IDM_DISK_ATTACH0     1201

#define IDM_VIEW_REFRESH     1301
#define IDM_VIEW_REGISTERS   1302

#define IDM_HELP_VERSION     1401
#define IDM_HELP_ABOUT       1402

static arm7vm_machine_t *g_machine = NULL;
static HFONT g_font = NULL;
static BOOL g_running = FALSE;
static BOOL g_prepared = FALSE;

typedef enum start_target {
    START_BIOS = 0,
    START_FLAT,
    START_BOOT,
    START_MONITOR
} start_target_t;

static start_target_t g_start_target = START_BIOS;
static char g_status[256] = "Ready. Built-in BIOS available; configure BOOT/MONITOR or attach disk0.";

static char g_bios_path[MAX_PATH] = "";
static BOOL g_use_embedded_bios = TRUE;
static char g_boot_path[MAX_PATH] = "";
static char g_monitor_path[MAX_PATH] = "";
static char g_flat_path[MAX_PATH] = "";

static uint32_t start_address(void)
{
    switch (g_start_target) {
    case START_FLAT:    return LOAD_ADDRESS;
    case START_BOOT:    return BOOT_ADDRESS;
    case START_MONITOR: return MONITOR_ADDRESS;
    case START_BIOS:
    default:            return BIOS_ADDRESS;
    }
}

static const char *start_name(void)
{
    switch (g_start_target) {
    case START_FLAT:    return "Flat";
    case START_BOOT:    return "BOOT";
    case START_MONITOR: return "Monitor";
    case START_BIOS:
    default:            return "BIOS";
    }
}

static int start_image_is_configured(void)
{
    switch (g_start_target) {
    case START_FLAT:    return g_flat_path[0] != '\0';
    case START_BOOT:    return g_boot_path[0] != '\0';
    case START_MONITOR: return g_monitor_path[0] != '\0';
    case START_BIOS:
    default:            return g_use_embedded_bios || g_bios_path[0] != '\0';
    }
}


static void set_status(HWND hwnd, const char *text)
{
    if (!text)
        text = "";
    strncpy(g_status, text, sizeof(g_status) - 1);
    g_status[sizeof(g_status) - 1] = '\0';
    InvalidateRect(hwnd, NULL, FALSE);
}

static int choose_file(HWND hwnd, const char *title, char *path, DWORD path_size)
{
    OPENFILENAMEA ofn;

    memset(&ofn, 0, sizeof(ofn));
    path[0] = '\0';

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrTitle = title;
    ofn.lpstrFile = path;
    ofn.nMaxFile = path_size;
    ofn.lpstrFilter =
        "Binary images (*.bin)\0*.bin\0"
        "Disk images (*.img)\0*.img\0"
        "All files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    return GetOpenFileNameA(&ofn) ? 1 : 0;
}

static void render_console(HDC hdc, RECT client)
{
    char line[CRT_COLS + 1];
    TEXTMETRICA tm;
    int row;
    int x = 10;
    int y = 10;
    int line_height = 16;

    SelectObject(hdc, g_font);
    SetBkColor(hdc, RGB(0, 0, 0));
    SetTextColor(hdc, RGB(220, 220, 220));

    if (GetTextMetricsA(hdc, &tm))
        line_height = tm.tmHeight;

    for (row = 0; row < CRT_ROWS; ++row) {
        int col;
        int last_non_space = -1;

        for (col = 0; col < CRT_COLS; ++col) {
            uint32_t addr =
                CRT_BASE +
                (uint32_t)(((row * CRT_COLS) + col) * CRT_CELL_BYTES);
            uint8_t ch = ' ';

            if (!g_machine ||
                !arm7vm_read_memory(g_machine, addr, &ch, 1) ||
                ch < 32 || ch > 126) {
                ch = ' ';
            }

            line[col] = (char)ch;
            if (ch != ' ')
                last_non_space = col;
        }

        line[last_non_space + 1] = '\0';

        if (line[0] != '\0')
            TextOutA(hdc, x, y + row * line_height,
                     line, (int)strlen(line));
    }

    SetTextColor(hdc, RGB(150, 150, 150));
    TextOutA(
        hdc,
        10,
        client.bottom - line_height - 8,
        g_status,
        (int)strlen(g_status)
    );
}

static HMENU build_menu(void)
{
    HMENU menu = CreateMenu();
    HMENU machine = CreatePopupMenu();
    HMENU start_at = CreatePopupMenu();
    HMENU firmware = CreatePopupMenu();
    HMENU disk = CreatePopupMenu();
    HMENU view = CreatePopupMenu();
    HMENU help = CreatePopupMenu();

    AppendMenuA(machine, MF_STRING, IDM_MACHINE_START, "Start\tF8");
    AppendMenuA(machine, MF_STRING, IDM_MACHINE_STOP, "Stop\tShift+F8");
    AppendMenuA(machine, MF_STRING, IDM_MACHINE_RESET, "Reset");

    AppendMenuA(start_at, MF_STRING | MF_CHECKED,
                IDM_MACHINE_START_BIOS, "BIOS       0x00000000");
    AppendMenuA(start_at, MF_STRING,
                IDM_MACHINE_START_FLAT, "Flat       0x00008000");
    AppendMenuA(start_at, MF_STRING,
                IDM_MACHINE_START_BOOT, "BOOT       0x00010000");
    AppendMenuA(start_at, MF_STRING,
                IDM_MACHINE_START_MONITOR, "Monitor    0x00020000");
    AppendMenuA(machine, MF_POPUP, (UINT_PTR)start_at, "Start At");

    AppendMenuA(machine, MF_SEPARATOR, 0, NULL);
    AppendMenuA(machine, MF_STRING, IDM_MACHINE_STEP, "Step One\tF10");
    AppendMenuA(machine, MF_STRING, IDM_MACHINE_RUN100, "Run 100 Instructions");
    AppendMenuA(machine, MF_STRING, IDM_MACHINE_RUN1000, "Run 1000 Instructions\tF9");
    AppendMenuA(machine, MF_STRING, IDM_MACHINE_RUN10000, "Run 10000 Instructions");
    AppendMenuA(machine, MF_SEPARATOR, 0, NULL);
    AppendMenuA(machine, MF_STRING, IDM_MACHINE_EXIT, "Exit");

    AppendMenuA(firmware, MF_STRING | MF_CHECKED,
                IDM_FIRMWARE_EMBEDDED_BIOS, "Embedded BIOS");
    AppendMenuA(firmware, MF_STRING, IDM_FIRMWARE_LOAD_BIOS,
                "Load BIOS at 0x00000000...");
    AppendMenuA(firmware, MF_STRING, IDM_FIRMWARE_LOAD_BOOT,
                "Load BOOT.BIN at 0x00010000...");
    AppendMenuA(firmware, MF_STRING, IDM_FIRMWARE_LOAD_MONITOR,
                "Load Monitor at 0x00020000...");
    AppendMenuA(firmware, MF_SEPARATOR, 0, NULL);
    AppendMenuA(firmware, MF_STRING, IDM_FIRMWARE_LOAD_FLAT,
                "Load Flat Binary at 0x00008000...\tCtrl+O");

    AppendMenuA(disk, MF_STRING, IDM_DISK_ATTACH0, "Attach disk0...");

    AppendMenuA(view, MF_STRING, IDM_VIEW_REFRESH, "Refresh\tF5");
    AppendMenuA(view, MF_STRING, IDM_VIEW_REGISTERS, "Registers");

    AppendMenuA(help, MF_STRING, IDM_HELP_VERSION, "Version");
    AppendMenuA(help, MF_STRING, IDM_HELP_ABOUT, "About");

    AppendMenuA(menu, MF_POPUP, (UINT_PTR)machine, "Machine");
    AppendMenuA(menu, MF_POPUP, (UINT_PTR)firmware, "Firmware");
    AppendMenuA(menu, MF_POPUP, (UINT_PTR)disk, "Disk");
    AppendMenuA(menu, MF_POPUP, (UINT_PTR)view, "View");
    AppendMenuA(menu, MF_POPUP, (UINT_PTR)help, "Help");

    return menu;
}

static void update_bios_menu_check(HWND hwnd)
{
    HMENU menu = GetMenu(hwnd);

    CheckMenuItem(
        menu,
        IDM_FIRMWARE_EMBEDDED_BIOS,
        MF_BYCOMMAND |
        (g_use_embedded_bios ? MF_CHECKED : MF_UNCHECKED)
    );
}

static void command_use_embedded_bios(HWND hwnd)
{
    g_use_embedded_bios = TRUE;
    g_prepared = FALSE;
    update_bios_menu_check(hwnd);
    set_status(hwnd,
               "BIOS: Embedded. Press Start to boot/reload.");
}

static int select_image(
    HWND hwnd,
    const char *title,
    char *path_store,
    size_t path_store_size,
    const char *kind,
    uint32_t address
)
{
    char path[MAX_PATH];
    char msg[640];

    if (!choose_file(hwnd, title, path, sizeof(path)))
        return 0;

    strncpy(path_store, path, path_store_size - 1);
    path_store[path_store_size - 1] = '\0';
    g_prepared = FALSE;

    snprintf(msg, sizeof(msg),
             "%s selected: %s -> 0x%08X. Press Start to boot/reload.",
             kind, path_store, address);
    set_status(hwnd, msg);
    return 1;
}

static void command_load_bios(HWND hwnd)
{
    if (select_image(hwnd,
                     "Select ARM7 BIOS image",
                     g_bios_path, sizeof(g_bios_path),
                     "BIOS", BIOS_ADDRESS)) {
        g_use_embedded_bios = FALSE;
        update_bios_menu_check(hwnd);
    }
}

static void command_load_boot(HWND hwnd)
{
    select_image(hwnd,
                 "Select ARM7 BOOT.BIN image",
                 g_boot_path, sizeof(g_boot_path),
                 "BOOT.BIN", BOOT_ADDRESS);
}

static void command_load_monitor(HWND hwnd)
{
    select_image(hwnd,
                 "Select ARM7 monitor image",
                 g_monitor_path, sizeof(g_monitor_path),
                 "Monitor", MONITOR_ADDRESS);
}

static void command_load_flat(HWND hwnd)
{
    select_image(hwnd,
                 "Select ARM7 flat binary",
                 g_flat_path, sizeof(g_flat_path),
                 "Flat binary", LOAD_ADDRESS);
}

static int load_configured_image(
    HWND hwnd,
    const char *path,
    uint32_t address,
    const char *kind
)
{
    char msg[640];

    if (!path[0])
        return 1;

    if (!arm7vm_load_binary(g_machine, path, address)) {
        snprintf(msg, sizeof(msg),
                 "Could not load %s: %s", kind, path);
        MessageBoxA(hwnd, msg, APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    return 1;
}

static int load_bios_image(HWND hwnd)
{
    char msg[640];

    /* Explicitly selected BIOS overrides the built-in default. */
    if (!g_use_embedded_bios) {
        return load_configured_image(
            hwnd,
            g_bios_path,
            BIOS_ADDRESS,
            "BIOS"
        );
    }

    if (!arm7vm_load_buffer(
            g_machine,
            default_bios,
            default_bios_size,
            BIOS_ADDRESS)) {
        snprintf(msg, sizeof(msg),
                 "Could not load built-in BIOS (%u bytes) at 0x%08X.",
                 (unsigned)default_bios_size,
                 BIOS_ADDRESS);
        MessageBoxA(hwnd, msg, APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    return 1;
}

static int prepare_machine(HWND hwnd)
{
    uint32_t entry = start_address();
    char msg[512];

    if (!start_image_is_configured()) {
        snprintf(msg, sizeof(msg),
                 "Start At is set to %s (0x%08X), but that image is not configured.",
                 start_name(), entry);
        MessageBoxA(hwnd, msg, APP_NAME, MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    arm7vm_reset(g_machine);

    /*
     * Reload every configured slot.  Loading order is intentionally fixed
     * and independent of the order in which the user selected the files.
     */
    if (!load_bios_image(hwnd))
        return 0;
    if (!load_configured_image(hwnd, g_flat_path,
                               LOAD_ADDRESS, "flat binary"))
        return 0;
    if (!load_configured_image(hwnd, g_boot_path,
                               BOOT_ADDRESS, "BOOT.BIN"))
        return 0;
    if (!load_configured_image(hwnd, g_monitor_path,
                               MONITOR_ADDRESS, "monitor"))
        return 0;

    if (!arm7vm_set_register(g_machine, 15u, entry)) {
        MessageBoxA(hwnd, "Images loaded, but PC could not be set.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        return 0;
    }

    g_prepared = TRUE;

    if (g_start_target == START_BIOS && g_use_embedded_bios) {
        snprintf(msg, sizeof(msg),
                 "Prepared: Built-in BIOS (%u bytes) -> PC 0x%08X.",
                 (unsigned)default_bios_size, entry);
    } else {
        snprintf(msg, sizeof(msg),
                 "Prepared: Start At %s -> PC 0x%08X.",
                 start_name(), entry);
    }
    set_status(hwnd, msg);
    return 1;
}

static void select_start_target(HWND hwnd, start_target_t target)
{
    HMENU menu = GetMenu(hwnd);
    UINT ids[] = {
        IDM_MACHINE_START_BIOS,
        IDM_MACHINE_START_FLAT,
        IDM_MACHINE_START_BOOT,
        IDM_MACHINE_START_MONITOR
    };
    unsigned i;
    char msg[192];

    g_start_target = target;
    g_prepared = FALSE;

    for (i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i)
        CheckMenuItem(menu, ids[i], MF_BYCOMMAND | MF_UNCHECKED);

    CheckMenuItem(
        menu,
        target == START_BIOS ? IDM_MACHINE_START_BIOS :
        target == START_FLAT ? IDM_MACHINE_START_FLAT :
        target == START_BOOT ? IDM_MACHINE_START_BOOT :
                               IDM_MACHINE_START_MONITOR,
        MF_BYCOMMAND | MF_CHECKED
    );

    snprintf(msg, sizeof(msg),
             "Start At: %s -> 0x%08X.",
             start_name(), start_address());
    set_status(hwnd, msg);
}

static void command_stop(HWND hwnd)
{
    if (!g_running) {
        set_status(hwnd, "Machine already stopped.");
        return;
    }

    g_running = FALSE;
    KillTimer(hwnd, RUN_TIMER_ID);
    set_status(hwnd, "Machine stopped; VM state preserved.");
}

static void command_start(HWND hwnd)
{
    if (g_running) {
        set_status(hwnd, "Machine is already running.");
        return;
    }

    if (!prepare_machine(hwnd))
        return;

    if (!SetTimer(hwnd, RUN_TIMER_ID, RUN_TIMER_MS, NULL)) {
        MessageBoxA(hwnd, "Could not start the ARM7 execution timer.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        return;
    }

    g_running = TRUE;

    {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "Running: Start At %s -> 0x%08X.",
                 start_name(), start_address());
        set_status(hwnd, msg);
    }
}

static void run_timer_slice(HWND hwnd)
{
    unsigned i;
    unsigned executed = 0;

    if (!g_running)
        return;

    for (i = 0; i < RUN_SLICE; ++i) {
        if (!arm7vm_step(g_machine)) {
            g_running = FALSE;
            KillTimer(hwnd, RUN_TIMER_ID);
            break;
        }
        ++executed;
    }

    InvalidateRect(hwnd, NULL, FALSE);

    if (!g_running) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "Execution stopped by VM after %u instruction%s in final slice.",
                 executed, executed == 1 ? "" : "s");
        set_status(hwnd, msg);
    }
}

static void command_attach_disk(HWND hwnd)
{
    char path[MAX_PATH];
    char msg[512];

    if (!choose_file(hwnd, "Attach ARM7 disk0 image", path, sizeof(path)))
        return;

    if (!arm7vm_attach_disk0(g_machine, path)) {
        MessageBoxA(hwnd, "Could not attach disk0.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        return;
    }

    snprintf(msg, sizeof(msg), "disk0 attached: %s", path);
    set_status(hwnd, msg);
}

static void command_step(HWND hwnd)
{
    if (g_running) {
        set_status(hwnd, "Stop the machine before single-stepping.");
        return;
    }

    if (!g_prepared && !prepare_machine(hwnd))
        return;

    if (!arm7vm_step(g_machine)) {
        set_status(hwnd, "Step stopped: VM reports no further execution.");
        return;
    }

    set_status(hwnd, "Executed one instruction.");
}

static void command_run_bounded(HWND hwnd, unsigned limit)
{
    unsigned i;
    if (g_running) {
        set_status(hwnd, "Stop the machine before using bounded execution.");
        return;
    }
    unsigned executed = 0;

    for (i = 0; i < limit; ++i) {
        if (!arm7vm_step(g_machine))
            break;
        ++executed;
    }

    {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "Run slice complete: %u/%u instruction%s executed.",
                 executed, limit, executed == 1 ? "" : "s");
        set_status(hwnd, msg);
    }
}


static void command_registers(HWND hwnd)
{
    /*
     * Layer 1 deliberately reuses the library's proven register dump.
     * A real graphical register pane can replace this once the public API
     * grows register-read accessors.
     */
    arm7vm_dump_registers(g_machine);
    MessageBoxA(
        hwnd,
        "Register state was emitted through the existing libarm7vm dump path.\n\n"
        "Start At and Step are now available for boot-chain debugging.\n"
        "The docked live register pane requires a public libarm7vm register-read\n"
        "accessor; this runner does not reach through the opaque VM boundary.",
        "ARM7 Registers",
        MB_OK | MB_ICONINFORMATION
    );
}

static int inject_keyboard_byte(HWND hwnd, uint8_t ch)
{
    char msg[160];

    if (!g_machine)
        return 0;

    if (!arm7vm_keyboard_push(g_machine, ch)) {
        set_status(hwnd, "Keyboard FIFO full; key was not queued.");
        return 0;
    }

    if (ch >= 32u && ch <= 126u) {
        snprintf(msg, sizeof(msg),
                 "Keyboard: queued 0x%02X ('%c'), pending=%u.",
                 (unsigned)ch, (char)ch,
                 arm7vm_keyboard_pending(g_machine));
    } else {
        snprintf(msg, sizeof(msg),
                 "Keyboard: queued 0x%02X, pending=%u.",
                 (unsigned)ch,
                 arm7vm_keyboard_pending(g_machine));
    }

    set_status(hwnd, msg);
    return 1;
}

static LRESULT CALLBACK wndproc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
)
{
    switch (msg) {
    case WM_CREATE:
        SetMenu(hwnd, build_menu());
        return 0;

    case WM_KEYDOWN:
        if ((GetKeyState(VK_CONTROL) & 0x8000) && (wparam == 'O')) {
            command_load_flat(hwnd);
            return 0;
        }
        if (wparam == VK_F8) {
            if (GetKeyState(VK_SHIFT) & 0x8000)
                command_stop(hwnd);
            else
                command_start(hwnd);
            return 0;
        }
        if (wparam == VK_F5) {
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        if (wparam == VK_F9) {
            command_run_bounded(hwnd, 1000);
            return 0;
        }
        if (wparam == VK_F10) {
            command_step(hwnd);
            return 0;
        }
        break;

    case WM_CHAR:
        /*
         * Layer 1 GUI keyboard path: inject text bytes only.
         *
         * Function keys and Ctrl+O remain handled by WM_KEYDOWN above.
         * Enter, Backspace and Tab are useful monitor characters, so retain
         * them; other control characters are ignored for now.
         */
        if (wparam <= 0xFFu) {
            uint8_t ch = (uint8_t)wparam;

            if ((ch >= 32u && ch <= 126u) ||
                ch == '\r' || ch == '\b' || ch == '\t') {
                inject_keyboard_byte(hwnd, ch);
                return 0;
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case IDM_MACHINE_START:
            command_start(hwnd);
            return 0;

        case IDM_MACHINE_STOP:
            command_stop(hwnd);
            return 0;

        case IDM_MACHINE_START_BIOS:
            select_start_target(hwnd, START_BIOS);
            return 0;

        case IDM_MACHINE_START_FLAT:
            select_start_target(hwnd, START_FLAT);
            return 0;

        case IDM_MACHINE_START_BOOT:
            select_start_target(hwnd, START_BOOT);
            return 0;

        case IDM_MACHINE_START_MONITOR:
            select_start_target(hwnd, START_MONITOR);
            return 0;

        case IDM_MACHINE_RESET:
            if (g_running) {
                g_running = FALSE;
                KillTimer(hwnd, RUN_TIMER_ID);
            }
            arm7vm_reset(g_machine);
            g_prepared = FALSE;
            set_status(hwnd, "Machine reset; firmware selections remain configured.");
            return 0;

        case IDM_MACHINE_STEP:
            command_step(hwnd);
            return 0;

        case IDM_MACHINE_RUN100:
            command_run_bounded(hwnd, 100);
            return 0;

        case IDM_MACHINE_RUN1000:
            command_run_bounded(hwnd, 1000);
            return 0;

        case IDM_MACHINE_RUN10000:
            command_run_bounded(hwnd, 10000);
            return 0;

        case IDM_MACHINE_EXIT:
            DestroyWindow(hwnd);
            return 0;

        case IDM_FIRMWARE_EMBEDDED_BIOS:
            command_use_embedded_bios(hwnd);
            return 0;

        case IDM_FIRMWARE_LOAD_BIOS:
            command_load_bios(hwnd);
            return 0;

        case IDM_FIRMWARE_LOAD_BOOT:
            command_load_boot(hwnd);
            return 0;

        case IDM_FIRMWARE_LOAD_MONITOR:
            command_load_monitor(hwnd);
            return 0;

        case IDM_FIRMWARE_LOAD_FLAT:
            command_load_flat(hwnd);
            return 0;

        case IDM_DISK_ATTACH0:
            command_attach_disk(hwnd);
            return 0;

        case IDM_VIEW_REFRESH:
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case IDM_VIEW_REGISTERS:
            command_registers(hwnd);
            return 0;

        case IDM_HELP_VERSION:
            MessageBoxA(hwnd,
                        APP_NAME " " APP_VERSION,
                        "Version",
                        MB_OK | MB_ICONINFORMATION);
            return 0;

        case IDM_HELP_ABOUT:
            MessageBoxA(
                hwnd,
                "arm7-runx\n\n"
                "Windows graphical developer shim for the ARM7 VM.\n"
                "Layer 4 Windows developer runner.\n\n"
                "Adds explicit BIOS / Flat / BOOT / Monitor Start At targets,\n"
                "preload slots, Start/Stop, single-step boot debugging,\n"
                "and host keyboard injection through the VM keyboard FIFO\n"
                "while keeping guest execution behind libarm7vm.\n\n"
                "Backed by libarm7vm.",
                "About arm7-runx",
                MB_OK | MB_ICONINFORMATION
            );
            return 0;
        }
        break;

    case WM_TIMER:
        if (wparam == RUN_TIMER_ID) {
            run_timer_slice(hwnd);
            return 0;
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT rc;
            HDC hdc = BeginPaint(hwnd, &ps);
            HDC back_dc = NULL;
            HBITMAP back_bitmap = NULL;
            HGDIOBJ old_bitmap = NULL;
            int width;
            int height;

            GetClientRect(hwnd, &rc);
            width = rc.right - rc.left;
            height = rc.bottom - rc.top;

            /*
             * Render the entire CRT into an off-screen bitmap, then copy the
             * completed frame to the window in one operation.  This prevents
             * the user from seeing the intermediate clear/redraw sequence.
             */
            if (width > 0 && height > 0) {
                back_dc = CreateCompatibleDC(hdc);
                if (back_dc)
                    back_bitmap = CreateCompatibleBitmap(hdc, width, height);

                if (back_dc && back_bitmap) {
                    old_bitmap = SelectObject(back_dc, back_bitmap);

                    FillRect(back_dc, &rc,
                             (HBRUSH)GetStockObject(BLACK_BRUSH));
                    render_console(back_dc, rc);

                    BitBlt(hdc, 0, 0, width, height,
                           back_dc, 0, 0, SRCCOPY);

                    SelectObject(back_dc, old_bitmap);
                } else {
                    /* Safe fallback if a GDI back buffer cannot be created. */
                    FillRect(hdc, &rc,
                             (HBRUSH)GetStockObject(BLACK_BRUSH));
                    render_console(hdc, rc);
                }
            }

            if (back_bitmap)
                DeleteObject(back_bitmap);
            if (back_dc)
                DeleteDC(back_dc);

            EndPaint(hwnd, &ps);
        }
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        if (g_running) {
            g_running = FALSE;
            KillTimer(hwnd, RUN_TIMER_ID);
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(
    HINSTANCE instance,
    HINSTANCE previous,
    LPSTR command_line,
    int show_command
)
{
    const char *class_name = "arm7_runx_window";
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;

    (void)previous;
    (void)command_line;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = wndproc;
    wc.hInstance = instance;
    wc.lpszClassName = class_name;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Could not register window class.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        return 1;
    }

    g_machine = arm7vm_create(DEFAULT_RAM);
    if (!g_machine) {
        MessageBoxA(NULL, "Could not create ARM7 virtual machine.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        return 1;
    }

    arm7vm_uart_init(0x09000000u);

    g_font = CreateFontA(
        -16, 0, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        FIXED_PITCH | FF_MODERN,
        "Consolas"
    );

    hwnd = CreateWindowExA(
        0,
        class_name,
        APP_NAME " " APP_VERSION,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        900,
        560,
        NULL,
        NULL,
        instance,
        NULL
    );

    if (!hwnd) {
        if (g_font) DeleteObject(g_font);
        arm7vm_destroy(g_machine);
        MessageBoxA(NULL, "Could not create main window.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, show_command);
    UpdateWindow(hwnd);

    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    if (g_font)
        DeleteObject(g_font);

    arm7vm_destroy(g_machine);
    g_machine = NULL;

    return (int)msg.wParam;
}
