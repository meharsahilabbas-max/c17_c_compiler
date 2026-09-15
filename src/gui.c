#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#define IDC_EDITOR 1001
#define IDC_OUTPUT 1002
#define IDC_OPEN 1003
#define IDC_SAVE 1004
#define IDC_BUILD 1005
#define IDC_NEW 1006
#define IDC_STATUS 1007
#define IDC_RUN 1008
#define IDC_OPTIMIZE 1009
#define IDM_NEW 2001
#define IDM_OPEN 2002
#define IDM_SAVE 2003
#define IDM_BUILD 2004
#define IDM_RUN 2005
#define IDM_EXIT 2006
#define IDM_ABOUT 2007

static HWND editor_window;
static HWND output_window;
static HWND status_window;
static HWND optimize_window;
static HFONT editor_font;
static wchar_t current_file[MAX_PATH];
static wchar_t compiler_path[MAX_PATH];

static void create_menu(HWND window) {
    HMENU bar = CreateMenu();
    HMENU file = CreatePopupMenu();
    HMENU build = CreatePopupMenu();
    HMENU help = CreatePopupMenu();
    AppendMenuW(file, MF_STRING, IDM_NEW, L"New\tCtrl+N");
    AppendMenuW(file, MF_STRING, IDM_OPEN, L"Open...\tCtrl+O");
    AppendMenuW(file, MF_STRING, IDM_SAVE, L"Save\tCtrl+S");
    AppendMenuW(file, MF_SEPARATOR, 0, NULL);
    AppendMenuW(file, MF_STRING, IDM_EXIT, L"Exit");
    AppendMenuW(build, MF_STRING, IDM_BUILD, L"Build\tF7");
    AppendMenuW(build, MF_STRING, IDM_RUN, L"Run\tF5");
    AppendMenuW(help, MF_STRING, IDM_ABOUT, L"About mycc IDE");
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)file, L"File");
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)build, L"Build");
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)help, L"Help");
    SetMenu(window, bar);
}

static void set_status(const wchar_t *text) { SetWindowTextW(status_window, text); }
static void append_output(const wchar_t *text) {
    int length = GetWindowTextLengthW(output_window);
    SendMessageW(output_window, EM_SETSEL, (WPARAM)length, (LPARAM)length);
    SendMessageW(output_window, EM_REPLACESEL, FALSE, (LPARAM)text);
}
static wchar_t *read_text_file(const wchar_t *path) {
    FILE *file = _wfopen(path, L"rb");
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return NULL; }
    long size = ftell(file);
    if (size < 0 || size > 50 * 1024 * 1024) { fclose(file); return NULL; }
    rewind(file);
    wchar_t *buffer = (wchar_t *)calloc((size_t)size + 1, sizeof(wchar_t));
    if (!buffer) { fclose(file); return NULL; }
    char *bytes = (char *)malloc((size_t)size + 1);
    if (!bytes || fread(bytes, 1, (size_t)size, file) != (size_t)size) { free(bytes); free(buffer); fclose(file); return NULL; }
    bytes[size] = 0;
    int converted = MultiByteToWideChar(CP_UTF8, 0, bytes, (int)size, buffer, (int)size + 1);
    if (converted == 0) MultiByteToWideChar(CP_ACP, 0, bytes, (int)size, buffer, (int)size + 1);
    free(bytes); fclose(file); return buffer;
}
static BOOL save_editor(const wchar_t *path) {
    int length = GetWindowTextLengthW(editor_window);
    wchar_t *text = (wchar_t *)calloc((size_t)length + 1, sizeof(wchar_t));
    if (!text) return FALSE;
    GetWindowTextW(editor_window, text, length + 1);
    int bytes_needed = WideCharToMultiByte(CP_UTF8, 0, text, length, NULL, 0, NULL, NULL);
    char *bytes = (char *)malloc((size_t)bytes_needed + 1);
    if (!bytes) { free(text); return FALSE; }
    WideCharToMultiByte(CP_UTF8, 0, text, length, bytes, bytes_needed, NULL, NULL);
    FILE *file = _wfopen(path, L"wb");
    BOOL ok = FALSE;
    if (file) { ok = fwrite(bytes, 1, (size_t)bytes_needed, file) == (size_t)bytes_needed; fclose(file); }
    free(bytes); free(text); return ok;
}
static void open_file(HWND owner) {
    OPENFILENAMEW dialog = {0}; dialog.lStructSize = sizeof(dialog);
    wchar_t path[MAX_PATH] = L"";
    dialog.hwndOwner = owner; dialog.lpstrFilter = L"C source\0*.c;*.h\0All files\0*.*\0";
    dialog.lpstrFile = path; dialog.nMaxFile = MAX_PATH; dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&dialog)) {
        wchar_t *text = read_text_file(path);
        if (!text) { MessageBoxW(owner, L"Could not read the selected file.", L"mycc IDE", MB_ICONERROR); return; }
        SetWindowTextW(editor_window, text); wcsncpy(current_file, path, MAX_PATH - 1); current_file[MAX_PATH - 1] = 0;
        free(text); set_status(L"File opened");
    }
}
static void save_file(HWND owner) {
    if (current_file[0] == 0) {
        OPENFILENAMEW dialog = {0}; dialog.lStructSize = sizeof(dialog); wchar_t path[MAX_PATH] = L"program.c";
        dialog.hwndOwner=owner; dialog.lpstrFilter=L"C source\0*.c\0All files\0*.*\0"; dialog.lpstrFile=path; dialog.nMaxFile=MAX_PATH; dialog.lpstrDefExt=L"c"; dialog.Flags=OFN_OVERWRITEPROMPT;
        if (!GetSaveFileNameW(&dialog)) return;
        wcsncpy(current_file, path, MAX_PATH - 1); current_file[MAX_PATH - 1] = 0;
    }
    if (save_editor(current_file)) set_status(L"File saved"); else MessageBoxW(owner, L"Could not save the file.", L"mycc IDE", MB_ICONERROR);
}
static void build_source(HWND owner) {
    save_file(owner);
    if (current_file[0] == 0) return;
    wchar_t executable[MAX_PATH]; wcsncpy(executable, current_file, MAX_PATH - 1); executable[MAX_PATH - 1] = 0;
    wchar_t *dot = wcsrchr(executable, L'.'); if (dot) wcscpy(dot, L".exe"); else wcscat_s(executable, MAX_PATH, L".exe");
    wchar_t command[3 * MAX_PATH];
    wchar_t optimization[8] = L"-O0";
    if (optimize_window) GetWindowTextW(optimize_window, optimization, (int)(sizeof(optimization) / sizeof(optimization[0])));
    _snwprintf_s(command, sizeof(command) / sizeof(command[0]), _TRUNCATE, L"\"%s\" %s \"%s\" -o \"%s\" 2>&1", compiler_path, optimization, current_file, executable);
    FILE *pipe = _wpopen(command, L"r");
    SetWindowTextW(output_window, L"");
    if (!pipe) { append_output(L"Could not start mycc.exe. Build the compiler first.\r\n"); set_status(L"Build failed"); return; }
    wchar_t line[512]; while (fgetws(line, (int)(sizeof(line) / sizeof(line[0])), pipe)) append_output(line);
    int result = _pclose(pipe);
    if (result == 0) { append_output(L"\r\nExecutable generated successfully.\r\n"); set_status(L"Build succeeded"); }
    else { append_output(L"\r\nCompilation failed.\r\n"); set_status(L"Build failed"); }
}
static void run_source(HWND owner) {
    build_source(owner);
    if (current_file[0] == 0) return;
    wchar_t executable[MAX_PATH]; wcsncpy(executable, current_file, MAX_PATH - 1); executable[MAX_PATH - 1] = 0;
    wchar_t *dot = wcsrchr(executable, L'.'); if (dot) wcscpy(dot, L".exe"); else wcscat_s(executable, MAX_PATH, L".exe");
    wchar_t command[MAX_PATH + 16]; _snwprintf_s(command, sizeof(command) / sizeof(command[0]), _TRUNCATE, L"\"%s\" 2>&1", executable);
    FILE *pipe = _wpopen(command, L"r"); if (!pipe) { append_output(L"Could not run the executable.\r\n"); set_status(L"Run failed"); return; }
    wchar_t line[512]; while (fgetws(line, (int)(sizeof(line) / sizeof(line[0])), pipe)) append_output(line);
    int result = _pclose(pipe); set_status(result == 0 ? L"Run succeeded" : L"Program returned a non-zero status");
}
static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: {
        editor_font = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Cascadia Mono");
        CreateWindowW(L"STATIC", L"mycc IDE", WS_CHILD | WS_VISIBLE, 16, 12, 240, 28, window, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"New", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, 10, 72, 30, window, (HMENU)IDC_NEW, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Open", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 360, 10, 72, 30, window, (HMENU)IDC_OPEN, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 440, 10, 72, 30, window, (HMENU)IDC_SAVE, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Build", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 520, 10, 84, 30, window, (HMENU)IDC_BUILD, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Run", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 612, 10, 72, 30, window, (HMENU)IDC_RUN, NULL, NULL);
        optimize_window = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 692, 10, 84, 180, window, (HMENU)IDC_OPTIMIZE, NULL, NULL);
        SendMessageW(optimize_window, CB_ADDSTRING, 0, (LPARAM)L"-O0"); SendMessageW(optimize_window, CB_ADDSTRING, 0, (LPARAM)L"-O1"); SendMessageW(optimize_window, CB_ADDSTRING, 0, (LPARAM)L"-O2"); SendMessageW(optimize_window, CB_ADDSTRING, 0, (LPARAM)L"-O3"); SendMessageW(optimize_window, CB_SETCURSEL, 0, 0);
        editor_window = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"int main() {\r\n    return 0;\r\n}\r\n", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL, 16, 54, 760, 430, window, (HMENU)IDC_EDITOR, NULL, NULL);
        output_window = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Ready.\r\n", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 16, 500, 760, 150, window, (HMENU)IDC_OUTPUT, NULL, NULL);
        status_window = CreateWindowW(L"STATIC", L"Ready", WS_CHILD | WS_VISIBLE, 16, 660, 760, 24, window, (HMENU)IDC_STATUS, NULL, NULL);
        SendMessageW(editor_window, WM_SETFONT, (WPARAM)editor_font, TRUE); SendMessageW(output_window, WM_SETFONT, (WPARAM)editor_font, TRUE);
        create_menu(window);
        return 0;
    }
    case WM_SIZE: { int width = LOWORD(lparam), height = HIWORD(lparam); MoveWindow(editor_window, 16, 54, width - 32, height - 250, TRUE); MoveWindow(output_window, 16, height - 180, width - 32, 150, TRUE); MoveWindow(status_window, 16, height - 24, width - 32, 20, TRUE); return 0; }
    case WM_COMMAND: switch (LOWORD(wparam)) { case IDC_NEW: case IDM_NEW: SetWindowTextW(editor_window, L"int main(void) {\r\n    return 0;\r\n}\r\n"); current_file[0]=0; set_status(L"New C17 file"); break; case IDC_OPEN: case IDM_OPEN: open_file(window); break; case IDC_SAVE: case IDM_SAVE: save_file(window); break; case IDC_BUILD: case IDM_BUILD: build_source(window); break; case IDC_RUN: case IDM_RUN: run_source(window); break; case IDM_EXIT: DestroyWindow(window); break; case IDM_ABOUT: MessageBoxW(window, L"mycc IDE\r\nC17 compiler workspace for Windows\r\n\r\nThe editor drives the mycc compiler and displays diagnostics.", L"About mycc IDE", MB_OK | MB_ICONINFORMATION); break; } return 0;
    case WM_DESTROY: DeleteObject(editor_font); PostQuitMessage(0); return 0;
    default: return DefWindowProcW(window, message, wparam, lparam);
    }
}
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show_command) {
    (void)previous; (void)command_line;
    GetModuleFileNameW(NULL, compiler_path, MAX_PATH); wchar_t *slash = wcsrchr(compiler_path, L'\\'); if (slash) wcscpy_s(slash + 1, MAX_PATH - (size_t)(slash + 1 - compiler_path), L"mycc.exe");
    INITCOMMONCONTROLSEX controls = { sizeof(controls), ICC_STANDARD_CLASSES }; InitCommonControlsEx(&controls);
    WNDCLASSW klass = { 0 }; klass.hInstance = instance; klass.lpfnWndProc = window_proc; klass.lpszClassName = L"myccIDE"; klass.hCursor = LoadCursor(NULL, IDC_ARROW); klass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); RegisterClassW(&klass);
    HWND window = CreateWindowExW(0, klass.lpszClassName, L"mycc IDE", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 820, 740, NULL, NULL, instance, NULL); if (!window) return 1;
    ShowWindow(window, show_command); UpdateWindow(window); MSG message; while (GetMessageW(&message, NULL, 0, 0) > 0) { TranslateMessage(&message); DispatchMessageW(&message); } return (int)message.wParam;
}
