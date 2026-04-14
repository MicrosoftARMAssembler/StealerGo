#pragma once

const std::string g_base64_chars = 
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

namespace crypto {
    class m_sha512 {
    public:
        static const size_t m_digest_size = 64;

        void init() {
            m_h[0] = 0x6a09e667f3bcc908ULL; m_h[1] = 0xbb67ae8584caa73bULL;
            m_h[2] = 0x3c6ef372fe94f82bULL; m_h[3] = 0xa54ff53a5f1d36f1ULL;
            m_h[4] = 0x510e527fade682d1ULL; m_h[5] = 0x9b05688c2b3e6c1fULL;
            m_h[6] = 0x1f83d9abfb41bd6bULL; m_h[7] = 0x5be0cd19137e2179ULL;
            m_bitlen = 0; m_datalen = 0;
        }

        void update(const uint8_t* data, size_t len) {
            for (size_t i = 0; i < len; ++i) {
                m_data_buf[m_datalen++] = data[i];
                if (m_datalen == 128) {
                    transform();
                    m_bitlen += 1024;
                    m_datalen = 0;
                }
            }
        }

        void finalize(uint8_t* out) {
            size_t i = m_datalen;
            if (m_datalen < 112) {
                m_data_buf[m_datalen++] = 0x80;
                while (m_datalen < 112) m_data_buf[m_datalen++] = 0x00;
            } else {
                m_data_buf[m_datalen++] = 0x80;
                while (m_datalen < 128) m_data_buf[m_datalen++] = 0x00;
                transform();
                memset(m_data_buf, 0, 112);
            }
            m_bitlen += m_datalen * 8;
            for (i = 0; i < 8; ++i)
                m_data_buf[112 + i] = (m_bitlen >> (56 - 8 * i)) & 0xff;
            transform();
            for (i = 0; i < 8; ++i) {
                out[i*8+0] = (m_h[i] >> 56) & 0xff;
                out[i*8+1] = (m_h[i] >> 48) & 0xff;
                out[i*8+2] = (m_h[i] >> 40) & 0xff;
                out[i*8+3] = (m_h[i] >> 32) & 0xff;
                out[i*8+4] = (m_h[i] >> 24) & 0xff;
                out[i*8+5] = (m_h[i] >> 16) & 0xff;
                out[i*8+6] = (m_h[i] >> 8) & 0xff;
                out[i*8+7] = m_h[i] & 0xff;
            }
        }

    private:
        uint64_t m_h[8];
        uint64_t m_bitlen;
        size_t m_datalen;
        uint8_t m_data_buf[128];

        static uint64_t rotr(uint64_t x, int n) { return (x >> n) | (x << (64 - n)); }

        void transform() {
            uint64_t w[80];
            for (int i = 0; i < 16; ++i) {
                w[i] = ((uint64_t)m_data_buf[i*8+0] << 56) | ((uint64_t)m_data_buf[i*8+1] << 48) |
                    ((uint64_t)m_data_buf[i*8+2] << 40) | ((uint64_t)m_data_buf[i*8+3] << 32) |
                    ((uint64_t)m_data_buf[i*8+4] << 24) | ((uint64_t)m_data_buf[i*8+5] << 16) |
                    ((uint64_t)m_data_buf[i*8+6] << 8)  | ((uint64_t)m_data_buf[i*8+7] << 0);
            }
            for (int i = 16; i < 80; ++i) {
                uint64_t s0 = rotr(w[i-15], 1) ^ rotr(w[i-15], 8) ^ (w[i-15] >> 7);
                uint64_t s1 = rotr(w[i-2], 19) ^ rotr(w[i-2], 61) ^ (w[i-2] >> 6);
                w[i] = w[i-16] + s0 + w[i-7] + s1;
            }

            uint64_t a = m_h[0], b = m_h[1], c = m_h[2], d = m_h[3];
            uint64_t e = m_h[4], f = m_h[5], g = m_h[6], hh = m_h[7];

            static const uint64_t K[80] = {
                0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
                0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
                0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
                0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
                0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
                0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
                0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
                0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
                0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
                0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
                0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
                0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
                0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
                0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
                0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
                0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
                0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
                0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
                0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
                0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
            };

            for (int i = 0; i < 80; ++i) {
                uint64_t s1 = rotr(e, 14) ^ rotr(e, 18) ^ rotr(e, 41);
                uint64_t ch = (e & f) ^ ((~e) & g);
                uint64_t temp1 = hh + s1 + ch + K[i] + w[i];
                uint64_t s0 = rotr(a, 28) ^ rotr(a, 34) ^ rotr(a, 39);
                uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint64_t temp2 = s0 + maj;

                hh = g; g = f; f = e; e = d + temp1;
                d = c; c = b; b = a; a = temp1 + temp2;
            }

            m_h[0] += a; m_h[1] += b; m_h[2] += c; m_h[3] += d;
            m_h[4] += e; m_h[5] += f; m_h[6] += g; m_h[7] += hh;
        }
    };

    void sha512(const uint8_t* data, size_t len, uint8_t* out) {
        m_sha512 ctx; ctx.init(); ctx.update(data, len); ctx.finalize(out);
    }


    class m_sha256 {
    public:
        static const size_t m_digest_size = 32;

        void init() {
            m_h[0] = 0x6a09e667; m_h[1] = 0xbb67ae85; m_h[2] = 0x3c6ef372; m_h[3] = 0xa54ff53a;
            m_h[4] = 0x510e527f; m_h[5] = 0x9b05688c; m_h[6] = 0x1f83d9ab; m_h[7] = 0x5be0cd19;
            m_bitlen = 0; m_datalen = 0;
        }

        void update(const uint8_t* data, size_t len) {
            for (size_t i = 0; i < len; ++i) {
                m_data_buf[m_datalen++] = data[i];
                if (m_datalen == 64) {
                    transform();
                    m_bitlen += 512;
                    m_datalen = 0;
                }
            }
        }

        void finalize(uint8_t* out) {
            size_t i = m_datalen;
            if (m_datalen < 56) {
                m_data_buf[m_datalen++] = 0x80;
                while (m_datalen < 56) m_data_buf[m_datalen++] = 0x00;
            } else {
                m_data_buf[m_datalen++] = 0x80;
                while (m_datalen < 64) m_data_buf[m_datalen++] = 0x00;
                transform();
                memset(m_data_buf, 0, 56);
            }
            m_bitlen += m_datalen * 8;
            for (i = 0; i < 8; ++i)
                m_data_buf[56 + i] = (m_bitlen >> (56 - 8 * i)) & 0xff;
            transform();
            for (i = 0; i < 8; ++i) {
                out[i*4+0] = (m_h[i] >> 24) & 0xff;
                out[i*4+1] = (m_h[i] >> 16) & 0xff;
                out[i*4+2] = (m_h[i] >> 8) & 0xff;
                out[i*4+3] = m_h[i] & 0xff;
            }
        }

    private:
        uint32_t m_h[8];
        uint64_t m_bitlen;
        size_t m_datalen;
        uint8_t m_data_buf[64];

        static uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

        void transform() {
            uint32_t w[64];
            for (int i = 0; i < 16; ++i) {
                w[i] = (m_data_buf[i*4+0] << 24) | (m_data_buf[i*4+1] << 16) |
                    (m_data_buf[i*4+2] << 8) | (m_data_buf[i*4+3] << 0);
            }
            for (int i = 16; i < 64; ++i) {
                uint32_t s0 = rotr(w[i-15], 7) ^ rotr(w[i-15], 18) ^ (w[i-15] >> 3);
                uint32_t s1 = rotr(w[i-2], 17) ^ rotr(w[i-2], 19) ^ (w[i-2] >> 10);
                w[i] = w[i-16] + s0 + w[i-7] + s1;
            }

            uint32_t a = m_h[0], b = m_h[1], c = m_h[2], d = m_h[3];
            uint32_t e = m_h[4], f = m_h[5], g = m_h[6], hh = m_h[7];

            static const uint32_t K[64] = {
                0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
                0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
                0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
                0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
                0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
                0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
                0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
                0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
            };

            for (int i = 0; i < 64; ++i) {
                uint32_t s1 = rotr(e,6) ^ rotr(e,11) ^ rotr(e,25);
                uint32_t ch = (e & f) ^ ((~e) & g);
                uint32_t temp1 = hh + s1 + ch + K[i] + w[i];
                uint32_t s0 = rotr(a,2) ^ rotr(a,13) ^ rotr(a,22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t temp2 = s0 + maj;

                hh = g; g = f; f = e; e = d + temp1;
                d = c; c = b; b = a; a = temp1 + temp2;
            }

            m_h[0] += a; m_h[1] += b; m_h[2] += c; m_h[3] += d;
            m_h[4] += e; m_h[5] += f; m_h[6] += g; m_h[7] += hh;
        }
    };

    void sha256(const uint8_t* data, size_t len, uint8_t* out) {
        m_sha256 ctx; ctx.init(); ctx.update(data, len); ctx.finalize(out);
    }

    void hmac_sha512(const uint8_t* key, size_t key_len,
        const uint8_t* data, size_t data_len, uint8_t* out) {
        uint8_t ipad[128] = {0}, opad[128] = {0}, key_buf[128] = {0};
        if (key_len > 128) {
            sha512(key, key_len, key_buf);
            key_len = 64;
            memcpy(key_buf, key_buf, key_len);
        } else {
            memcpy(key_buf, key, key_len);
        }
        for (int i = 0; i < 128; ++i) {
            ipad[i] = key_buf[i] ^ 0x36;
            opad[i] = key_buf[i] ^ 0x5c;
        }
        uint8_t inner[64];
        m_sha512 ctx;
        ctx.init(); ctx.update(ipad, 128); ctx.update(data, data_len); ctx.finalize(inner);
        ctx.init(); ctx.update(opad, 128); ctx.update(inner, 64); ctx.finalize(out);
    }

    void bip32_master_from_seed(const uint8_t* seed, size_t seed_len,
        uint8_t* master_priv, uint8_t* chain_code) {
        const uint8_t* key = (const uint8_t*)oxorany("Bitcoin seed");
        uint8_t out[64];
        hmac_sha512(key, 12, seed, seed_len, out);
        memcpy(master_priv, out, 32);
        memcpy(chain_code, out + 32, 32);
    }

    bool inflate_gzip(const std::vector<uint8_t>& compressed,
        std::vector<uint8_t>& decompressed) {
        if (compressed.size() < 5) {
            std::cerr << oxorany("Compressed data too short") << std::endl;
            return false;
        }

        const uint8_t* gzip_data = compressed.data() + 4;
        size_t gzip_len = compressed.size() - 4;

        if (gzip_len < 2 || gzip_data[0] != 0x1F || gzip_data[1] != 0x8B) {
            std::cerr << oxorany("Not a valid gzip stream (magic: ")
                << std::hex << (int)gzip_data[0] << " " << (int)gzip_data[1] << ")"
                << std::dec << std::endl;
            return false;
        }

        z_stream strm = {0};
        strm.next_in = (Bytef*)gzip_data;
        strm.avail_in = (uInt)gzip_len;

        int ret = inflateInit2(&strm, 15 + 16);
        if (ret != Z_OK) {
            std::cerr << oxorany("inflateInit2 failed: ") << ret << std::endl;
            return false;
        }

        size_t out_size = gzip_len * 2;
        decompressed.resize(out_size);
        strm.next_out = decompressed.data();
        strm.avail_out = (uInt)out_size;

        ret = inflate(&strm, Z_FINISH);
        if (ret == Z_STREAM_END) {
            decompressed.resize(strm.total_out);
            inflateEnd(&strm);
            return true;
        } else if (ret == Z_BUF_ERROR && strm.avail_out == 0) {
            out_size = gzip_len * 4;
            decompressed.resize(out_size);
            strm.next_out = decompressed.data() + strm.total_out;
            strm.avail_out = (uInt)(out_size - strm.total_out);
            ret = inflate(&strm, Z_FINISH);
            if (ret == Z_STREAM_END) {
                decompressed.resize(strm.total_out);
                inflateEnd(&strm);
                return true;
            }
        }

        std::cerr << oxorany("Decompression failed: ret=") << ret;
        if (strm.msg) std::cerr << oxorany(" (") << strm.msg << oxorany(")");
        std::cerr << std::endl;
        inflateEnd(&strm);
        return false;
    }

    uint32_t sha256_checksum_bits(const std::vector<uint8_t>& data, int bits) {
        std::vector<uint8_t> hash(32);
        sha256(data.data(), data.size(), hash.data());
        uint32_t checksum = 0;
        for (int i = 0; i < bits; ++i) {
            int byte = i / 8;
            int bit = 7 - (i % 8);
            if (hash[byte] & (1 << bit))
                checksum |= (1 << (bits - 1 - i));
        }
        return checksum;
    }

    bool aes_gcm_decrypt(const uint8_t* key, size_t key_len,
        const uint8_t* iv, size_t iv_len,
        const uint8_t* auth_tag, size_t auth_len,
        const uint8_t* ciphertext, size_t ciphertext_len,
        std::vector<uint8_t>& plaintext) {
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        BCRYPT_KEY_HANDLE hKey = nullptr;
        NTSTATUS status;

        status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
        if (!BCRYPT_SUCCESS(status)) {
            std::cerr << oxorany("BCryptOpenAlgorithmProvider failed: 0x") << std::hex << status << std::dec << std::endl;
            return false;
        }

        status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE,
            (PUCHAR)BCRYPT_CHAIN_MODE_GCM,
            sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
        if (!BCRYPT_SUCCESS(status)) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        status = BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0,
            (PUCHAR)key, (ULONG)key_len, 0);
        if (!BCRYPT_SUCCESS(status)) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
        BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
        authInfo.pbNonce = (PUCHAR)iv;
        authInfo.cbNonce = (ULONG)iv_len;
        authInfo.pbTag = (PUCHAR)auth_tag;
        authInfo.cbTag = (ULONG)auth_len;

        plaintext.resize(ciphertext_len);
        ULONG bytesDecrypted = 0;
        status = BCryptDecrypt(hKey,
            (PUCHAR)ciphertext, (ULONG)ciphertext_len,
            &authInfo,
            nullptr, 0,
            plaintext.data(), (ULONG)plaintext.size(),
            &bytesDecrypted, 0);
        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);

        if (!BCRYPT_SUCCESS(status) || bytesDecrypted != ciphertext_len) {
            plaintext.clear();
            return false;
        }
        return true;
    }


    static inline bool is_base64(unsigned char c) {
        return (isalnum(c) || (c == '+') || (c == '/'));
    }

    std::vector<uint8_t> base64_decode(const std::string& encoded_string) {
        size_t in_len = encoded_string.size();
        size_t i = 0;
        size_t j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<uint8_t> ret;

        while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
            char_array_4[i++] = encoded_string[in_]; in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = static_cast<unsigned char>(g_base64_chars.find(char_array_4[i]));

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0x0f) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x03) << 6) + char_array_4[3];

                for (i = 0; i < 3; i++)
                    ret.push_back(char_array_3[i]);
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 4; j++)
                char_array_4[j] = 0;

            for (j = 0; j < 4; j++)
                char_array_4[j] = static_cast<unsigned char>(g_base64_chars.find(char_array_4[j]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0x0f) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x03) << 6) + char_array_4[3];

            for (j = 0; j < i - 1; j++)
                ret.push_back(char_array_3[j]);
        }

        return ret;
    }

    std::string url_decode(const std::string& encoded) {
        std::string decoded;
        for (size_t i = 0; i < encoded.length(); ++i) {
            if (encoded[i] == '%') {
                if (i + 2 < encoded.length()) {
                    int value = 0;
                    std::istringstream hex(encoded.substr(i+1, 2));
                    hex >> std::hex >> value;
                    decoded += static_cast<char>(value);
                    i += 2;
                } else {
                    decoded += encoded[i]; // malformed, keep as is
                }
            } else if (encoded[i] == '+') {
                decoded += ' ';
            } else {
                decoded += encoded[i];
            }
        }
        return decoded;
    }
}