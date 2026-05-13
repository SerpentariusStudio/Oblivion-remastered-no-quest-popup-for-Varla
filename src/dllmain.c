/*
 * NoQuestPopup - version.dll proxy for Oblivion Remastered
 *
 * Suppresses all quest popup notifications (Added/Updated/Completed)
 * by patching a JZ -> JMP in the quest notification handler at runtime.
 *
 * Install: drop version.dll into OblivionRemastered\Binaries\Win64\
 * Uninstall: delete the DLL
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

/* ========================================================================
 * Logging
 * ======================================================================== */

static void log_message(const char *fmt, ...)
{
    FILE *f = fopen("NoQuestPopup.log", "a");
    if (!f) return;

    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond);

    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);

    fprintf(f, "\n");
    fclose(f);
}

/* ========================================================================
 * version.dll proxy forwarding (17 exports)
 * ======================================================================== */

static HMODULE g_realVersion = NULL;

/* Function pointer types */
typedef BOOL (WINAPI *GetFileVersionInfoA_t)(LPCSTR, DWORD, DWORD, LPVOID);
typedef BOOL (WINAPI *GetFileVersionInfoW_t)(LPCWSTR, DWORD, DWORD, LPVOID);
typedef BOOL (WINAPI *GetFileVersionInfoExA_t)(DWORD, LPCSTR, DWORD, DWORD, LPVOID);
typedef BOOL (WINAPI *GetFileVersionInfoExW_t)(DWORD, LPCWSTR, DWORD, DWORD, LPVOID);
typedef DWORD (WINAPI *GetFileVersionInfoSizeA_t)(LPCSTR, LPDWORD);
typedef DWORD (WINAPI *GetFileVersionInfoSizeW_t)(LPCWSTR, LPDWORD);
typedef DWORD (WINAPI *GetFileVersionInfoSizeExA_t)(DWORD, LPCSTR, LPDWORD);
typedef DWORD (WINAPI *GetFileVersionInfoSizeExW_t)(DWORD, LPCWSTR, LPDWORD);
typedef int (WINAPI *GetFileVersionInfoByHandle_t)(DWORD, LPCVOID);
typedef DWORD (WINAPI *VerFindFileA_t)(DWORD, LPCSTR, LPCSTR, LPCSTR, LPSTR, PUINT, LPSTR, PUINT);
typedef DWORD (WINAPI *VerFindFileW_t)(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT, LPWSTR, PUINT);
typedef DWORD (WINAPI *VerInstallFileA_t)(DWORD, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPSTR, PUINT);
typedef DWORD (WINAPI *VerInstallFileW_t)(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT);
typedef DWORD (WINAPI *VerLanguageNameA_t)(DWORD, LPSTR, DWORD);
typedef DWORD (WINAPI *VerLanguageNameW_t)(DWORD, LPWSTR, DWORD);
typedef BOOL (WINAPI *VerQueryValueA_t)(LPCVOID, LPCSTR, LPVOID *, PUINT);
typedef BOOL (WINAPI *VerQueryValueW_t)(LPCVOID, LPCWSTR, LPVOID *, PUINT);

/* Function pointers to real version.dll */
static GetFileVersionInfoA_t       p_GetFileVersionInfoA;
static GetFileVersionInfoW_t       p_GetFileVersionInfoW;
static GetFileVersionInfoExA_t     p_GetFileVersionInfoExA;
static GetFileVersionInfoExW_t     p_GetFileVersionInfoExW;
static GetFileVersionInfoSizeA_t   p_GetFileVersionInfoSizeA;
static GetFileVersionInfoSizeW_t   p_GetFileVersionInfoSizeW;
static GetFileVersionInfoSizeExA_t p_GetFileVersionInfoSizeExA;
static GetFileVersionInfoSizeExW_t p_GetFileVersionInfoSizeExW;
static GetFileVersionInfoByHandle_t p_GetFileVersionInfoByHandle;
static VerFindFileA_t              p_VerFindFileA;
static VerFindFileW_t              p_VerFindFileW;
static VerInstallFileA_t           p_VerInstallFileA;
static VerInstallFileW_t           p_VerInstallFileW;
static VerLanguageNameA_t          p_VerLanguageNameA;
static VerLanguageNameW_t          p_VerLanguageNameW;
static VerQueryValueA_t            p_VerQueryValueA;
static VerQueryValueW_t            p_VerQueryValueW;

/* Proxy wrapper functions */

__declspec(dllexport) BOOL WINAPI proxy_GetFileVersionInfoA(LPCSTR fn, DWORD h, DWORD sz, LPVOID d)
{ return p_GetFileVersionInfoA(fn, h, sz, d); }

__declspec(dllexport) BOOL WINAPI proxy_GetFileVersionInfoW(LPCWSTR fn, DWORD h, DWORD sz, LPVOID d)
{ return p_GetFileVersionInfoW(fn, h, sz, d); }

__declspec(dllexport) BOOL WINAPI proxy_GetFileVersionInfoExA(DWORD fl, LPCSTR fn, DWORD h, DWORD sz, LPVOID d)
{ return p_GetFileVersionInfoExA(fl, fn, h, sz, d); }

__declspec(dllexport) BOOL WINAPI proxy_GetFileVersionInfoExW(DWORD fl, LPCWSTR fn, DWORD h, DWORD sz, LPVOID d)
{ return p_GetFileVersionInfoExW(fl, fn, h, sz, d); }

__declspec(dllexport) DWORD WINAPI proxy_GetFileVersionInfoSizeA(LPCSTR fn, LPDWORD h)
{ return p_GetFileVersionInfoSizeA(fn, h); }

__declspec(dllexport) DWORD WINAPI proxy_GetFileVersionInfoSizeW(LPCWSTR fn, LPDWORD h)
{ return p_GetFileVersionInfoSizeW(fn, h); }

__declspec(dllexport) DWORD WINAPI proxy_GetFileVersionInfoSizeExA(DWORD fl, LPCSTR fn, LPDWORD h)
{ return p_GetFileVersionInfoSizeExA(fl, fn, h); }

__declspec(dllexport) DWORD WINAPI proxy_GetFileVersionInfoSizeExW(DWORD fl, LPCWSTR fn, LPDWORD h)
{ return p_GetFileVersionInfoSizeExW(fl, fn, h); }

__declspec(dllexport) int WINAPI proxy_GetFileVersionInfoByHandle(DWORD a, LPCVOID b)
{ return p_GetFileVersionInfoByHandle(a, b); }

__declspec(dllexport) DWORD WINAPI proxy_VerFindFileA(DWORD fl, LPCSTR fn, LPCSTR wr, LPCSTR ad, LPSTR cd, PUINT cdl, LPSTR df, PUINT dfl)
{ return p_VerFindFileA(fl, fn, wr, ad, cd, cdl, df, dfl); }

__declspec(dllexport) DWORD WINAPI proxy_VerFindFileW(DWORD fl, LPCWSTR fn, LPCWSTR wr, LPCWSTR ad, LPWSTR cd, PUINT cdl, LPWSTR df, PUINT dfl)
{ return p_VerFindFileW(fl, fn, wr, ad, cd, cdl, df, dfl); }

__declspec(dllexport) DWORD WINAPI proxy_VerInstallFileA(DWORD fl, LPCSTR sf, LPCSTR df, LPCSTR sr, LPCSTR dr, LPCSTR cd, LPSTR tf, PUINT tfl)
{ return p_VerInstallFileA(fl, sf, df, sr, dr, cd, tf, tfl); }

__declspec(dllexport) DWORD WINAPI proxy_VerInstallFileW(DWORD fl, LPCWSTR sf, LPCWSTR df, LPCWSTR sr, LPCWSTR dr, LPCWSTR cd, LPWSTR tf, PUINT tfl)
{ return p_VerInstallFileW(fl, sf, df, sr, dr, cd, tf, tfl); }

__declspec(dllexport) DWORD WINAPI proxy_VerLanguageNameA(DWORD lang, LPSTR buf, DWORD sz)
{ return p_VerLanguageNameA(lang, buf, sz); }

__declspec(dllexport) DWORD WINAPI proxy_VerLanguageNameW(DWORD lang, LPWSTR buf, DWORD sz)
{ return p_VerLanguageNameW(lang, buf, sz); }

__declspec(dllexport) BOOL WINAPI proxy_VerQueryValueA(LPCVOID blk, LPCSTR sub, LPVOID *buf, PUINT len)
{ return p_VerQueryValueA(blk, sub, buf, len); }

__declspec(dllexport) BOOL WINAPI proxy_VerQueryValueW(LPCVOID blk, LPCWSTR sub, LPVOID *buf, PUINT len)
{ return p_VerQueryValueW(blk, sub, buf, len); }

/* Load the real version.dll from System32 */
static BOOL load_real_version(void)
{
    char sysdir[MAX_PATH];
    char path[MAX_PATH];

    GetSystemDirectoryA(sysdir, MAX_PATH);
    snprintf(path, MAX_PATH, "%s\\version.dll", sysdir);

    g_realVersion = LoadLibraryA(path);
    if (!g_realVersion) {
        log_message("ERROR: Failed to load real version.dll from %s", path);
        return FALSE;
    }

    p_GetFileVersionInfoA       = (GetFileVersionInfoA_t)       GetProcAddress(g_realVersion, "GetFileVersionInfoA");
    p_GetFileVersionInfoW       = (GetFileVersionInfoW_t)       GetProcAddress(g_realVersion, "GetFileVersionInfoW");
    p_GetFileVersionInfoExA     = (GetFileVersionInfoExA_t)     GetProcAddress(g_realVersion, "GetFileVersionInfoExA");
    p_GetFileVersionInfoExW     = (GetFileVersionInfoExW_t)     GetProcAddress(g_realVersion, "GetFileVersionInfoExW");
    p_GetFileVersionInfoSizeA   = (GetFileVersionInfoSizeA_t)   GetProcAddress(g_realVersion, "GetFileVersionInfoSizeA");
    p_GetFileVersionInfoSizeW   = (GetFileVersionInfoSizeW_t)   GetProcAddress(g_realVersion, "GetFileVersionInfoSizeW");
    p_GetFileVersionInfoSizeExA = (GetFileVersionInfoSizeExA_t) GetProcAddress(g_realVersion, "GetFileVersionInfoSizeExA");
    p_GetFileVersionInfoSizeExW = (GetFileVersionInfoSizeExW_t) GetProcAddress(g_realVersion, "GetFileVersionInfoSizeExW");
    p_GetFileVersionInfoByHandle= (GetFileVersionInfoByHandle_t)GetProcAddress(g_realVersion, "GetFileVersionInfoByHandle");
    p_VerFindFileA              = (VerFindFileA_t)              GetProcAddress(g_realVersion, "VerFindFileA");
    p_VerFindFileW              = (VerFindFileW_t)              GetProcAddress(g_realVersion, "VerFindFileW");
    p_VerInstallFileA           = (VerInstallFileA_t)           GetProcAddress(g_realVersion, "VerInstallFileA");
    p_VerInstallFileW           = (VerInstallFileW_t)           GetProcAddress(g_realVersion, "VerInstallFileW");
    p_VerLanguageNameA          = (VerLanguageNameA_t)          GetProcAddress(g_realVersion, "VerLanguageNameA");
    p_VerLanguageNameW          = (VerLanguageNameW_t)          GetProcAddress(g_realVersion, "VerLanguageNameW");
    p_VerQueryValueA            = (VerQueryValueA_t)            GetProcAddress(g_realVersion, "VerQueryValueA");
    p_VerQueryValueW            = (VerQueryValueW_t)            GetProcAddress(g_realVersion, "VerQueryValueW");

    log_message("Loaded real version.dll from %s", path);
    return TRUE;
}

/* ========================================================================
 * Pattern scanner + patcher
 * ======================================================================== */

/*
 * We scan for the following byte pattern in the quest notification handler:
 *
 *   48 89 03               MOV  [RBX], RAX      (3 bytes) - context before
 *   E8 ?? ?? ?? ??         CALL FUN_14662f130    (5 bytes)
 *   84 C0                  TEST AL, AL           (2 bytes)
 *   0F 84 ?? ?? ?? ??      JZ   <target>         (6 bytes) <-- patch this
 *   49 8B D5               MOV  RDX, R13         (3 bytes)
 *   49 8B CF               MOV  RCX, R15         (3 bytes) - context after
 *
 * The JZ (0F 84) is replaced with JMP (E9) + adjusted offset + NOP:
 *   Original: 0F 84 xx xx xx xx  (6 bytes, offset is from end of instruction)
 *   Patched:  E9 yy yy yy yy 90  (5-byte JMP + 1 NOP)
 *   where yy = xx + 1  (JMP is 5 bytes vs JZ's 6, so offset increases by 1)
 */

static const int SIG_PATTERN[] = {
    0x48, 0x89, 0x03,                /* MOV [RBX], RAX  - context */
    0xE8, -1, -1, -1, -1,           /* CALL ????????    */
    0x84, 0xC0,                      /* TEST AL, AL      */
    0x0F, 0x84, -1, -1, -1, -1,     /* JZ ????????      */
    0x49, 0x8B, 0xD5,               /* MOV RDX, R13     */
    0x49, 0x8B, 0xCF                /* MOV RCX, R15 - context */
};
#define SIG_PATTERN_LEN (sizeof(SIG_PATTERN) / sizeof(SIG_PATTERN[0]))
static const int JZ_OFFSET_IN_PATTERN = 10; /* offset of 0F 84 within pattern */

static BYTE *find_pattern(BYTE *base, SIZE_T size)
{
    if (size < (SIZE_T)SIG_PATTERN_LEN) return NULL;

    SIZE_T limit = size - SIG_PATTERN_LEN;
    for (SIZE_T i = 0; i <= limit; i++) {
        BOOL match = TRUE;
        for (int j = 0; j < SIG_PATTERN_LEN; j++) {
            if (SIG_PATTERN[j] != -1 && base[i + j] != (BYTE)SIG_PATTERN[j]) {
                match = FALSE;
                break;
            }
        }
        if (match) return &base[i];
    }
    return NULL;
}

static BOOL apply_patch(BYTE *jz_addr)
{
    LONG original_offset;
    memcpy(&original_offset, jz_addr + 2, sizeof(LONG));
    LONG new_offset = original_offset + 1;

    DWORD old_protect;
    if (!VirtualProtect(jz_addr, 6, PAGE_EXECUTE_READWRITE, &old_protect)) {
        log_message("ERROR: VirtualProtect failed (error %lu)", GetLastError());
        return FALSE;
    }

    jz_addr[0] = 0xE9;
    memcpy(jz_addr + 1, &new_offset, sizeof(LONG));
    jz_addr[5] = 0x90;

    VirtualProtect(jz_addr, 6, old_protect, &old_protect);
    return TRUE;
}

static DWORD WINAPI patch_thread(LPVOID param)
{
    (void)param;

    log_message("NoQuestPopup v1.0 - patch thread started");

    /* Wait briefly for the game to finish loading */
    Sleep(3000);

    HMODULE exe = GetModuleHandleA(NULL);
    if (!exe) {
        log_message("ERROR: GetModuleHandleA(NULL) failed");
        return 1;
    }

    BYTE *base = (BYTE *)exe;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    SIZE_T image_size = nt->OptionalHeader.SizeOfImage;

    log_message("Game module: base=0x%p, size=0x%zX", base, image_size);

    BYTE *match = find_pattern(base, image_size);
    if (!match) {
        log_message("ERROR: Pattern not found! Game may have been updated.");
        return 1;
    }

    BYTE *jz_addr = match + JZ_OFFSET_IN_PATTERN;
    log_message("Pattern found at 0x%p, JZ instruction at 0x%p (RVA: 0x%zX)",
                match, jz_addr, (SIZE_T)(jz_addr - base));

    if (jz_addr[0] == 0xE9) {
        log_message("Already patched (JMP found) - skipping");
        return 0;
    }

    if (jz_addr[0] != 0x0F || jz_addr[1] != 0x84) {
        log_message("ERROR: Expected JZ (0F 84) at patch site, found %02X %02X",
                    jz_addr[0], jz_addr[1]);
        return 1;
    }

    if (!apply_patch(jz_addr)) {
        log_message("ERROR: Failed to apply patch");
        return 1;
    }

    log_message("SUCCESS: Patched JZ -> JMP at 0x%p (quest popups suppressed)", jz_addr);
    return 0;
}

/* ========================================================================
 * DLL entry point
 * ======================================================================== */

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    (void)hModule;
    (void)lpReserved;

    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);

        {
            FILE *f = fopen("NoQuestPopup.log", "w");
            if (f) fclose(f);
        }

        if (!load_real_version())
            return FALSE;

        CreateThread(NULL, 0, patch_thread, NULL, 0, NULL);
        break;

    case DLL_PROCESS_DETACH:
        if (g_realVersion) {
            FreeLibrary(g_realVersion);
            g_realVersion = NULL;
        }
        break;
    }

    return TRUE;
}
