#pragma once

void registryCheck() {
	std::string cmd = "REG QUERY HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet001\\Control\\Class\\{4D36E968-E325-11CE-BFC1-08002BE10318}\\0000\\";
	if (system((cmd + "DriverDesc").c_str()) == 0 || system((cmd + "ProviderName").c_str()) == 0) {
		exit(0);
	}
}

void processesAndFilesCheck() {
	std::string vmwareDll = getenv("SystemRoot");
	vmwareDll += "\\System32\\vmGuestLib.dll";
	std::string virtualboxDll = getenv("SystemRoot");
	virtualboxDll += "\\vboxmrxnp.dll";

	std::string process;
	FILE* pipe = _popen("TASKLIST /FI \"STATUS eq RUNNING\" | find /V \"Image Name\" | find /V \"=\"", "r");
	char buffer[128];
	while (fgets(buffer, 128, pipe) != NULL) {
		process += buffer;
	}
	_pclose(pipe);

	std::vector<std::string> processList;
	size_t pos = 0;
	while ((pos = process.find(".exe", pos)) != std::string::npos) {
		processList.push_back(process.substr(0, pos + 4));
		pos += 4;
	}

	if (std::find(processList.begin(), processList.end(), "VMwareService.exe") != processList.end() ||
		std::find(processList.begin(), processList.end(), "VMwareTray.exe") != processList.end()) {
		exit(0);
	}

	if (GetFileAttributesA(vmwareDll.c_str()) != INVALID_FILE_ATTRIBUTES) {
		exit(0);
	}

	if (GetFileAttributesA(virtualboxDll.c_str()) != INVALID_FILE_ATTRIBUTES) {
		exit(0);
	}

	HMODULE sandboxieDll = LoadLibrary(L"SbieDll.dll");
	if (sandboxieDll != NULL) {
		FreeLibrary(sandboxieDll);
		exit(0);
	}

	std::ifstream processListFile("https://pastebin.com/raw/Xe5WMsTU");
	std::string processl;
	while (std::getline(processListFile, processl)) {
		if (std::find(processList.begin(), processList.end(), processl) != processList.end()) {
			exit(0);
		}
	}
}

void macCheck() {
	std::string macAddress = "00:00:00:00:00:00"; // Placeholder for getting MAC Address
	std::ifstream macListFile("https://pastebin.com/raw/ftd50eAq");
	std::string mac;
	while (std::getline(macListFile, mac)) {
		if (macAddress.substr(0, 8) == mac) {
			exit(0);
		}
	}
}

void checkPc() {
	std::string vmName;
	std::string vmNameFileContent;
	std::ifstream vmNameFile("https://pastebin.com/raw/jnEaykLU");
	std::getline(vmNameFile, vmNameFileContent);
	if (vmName == vmNameFileContent) {
		exit(0);
	}

	std::string vmUsername;
	std::string hostName;
	char hostNameBuffer[1024];
	gethostname(hostNameBuffer, 1024);
	hostName = hostNameBuffer;
	std::ifstream vmUsernameFile("https://pastebin.com/raw/Vgztru48");
	std::getline(vmUsernameFile, vmUsername);
	if (hostName == vmUsername) {
		exit(0);
	}
}

void hwidVm() {
	std::string currentMachineId; // Placeholder for getting machine ID
	std::ifstream hwidVmFile("https://pastebin.com/raw/E5pwq7NH");
	std::string hwid;
	while (std::getline(hwidVmFile, hwid)) {
		if (currentMachineId == hwid) {
			exit(0);
		}
	}
}

void checkGpu() {
	std::string gpuDescription;
	std::ifstream gpuListFile("https://pastebin.com/raw/LSpkCfqc");
	std::string gpu;
	while (std::getline(gpuListFile, gpu)) {
		if (gpuDescription == gpu) {
			exit(0);
		}
	}
}

void checkIp() {
	std::ifstream ipListFile("https://pastebin.com/raw/tDXxxRUc");
	std::vector<std::string> ipList;
	std::string ip;
	while (std::getline(ipListFile, ip)) {
		ipList.push_back(ip);
	}

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return;
	}

	char ipBuffer[100];
	if (gethostname(ipBuffer, sizeof(ipBuffer)) == SOCKET_ERROR) {
		WSACleanup();
		return;
	}
	hostent* host;
	host = gethostbyname(ipBuffer);
	if (host == NULL) {
		WSACleanup();
		return;
	}
	std::string ipAddr = inet_ntoa(*(struct in_addr*)*host->h_addr_list);

	if (std::find(ipList.begin(), ipList.end(), ipAddr) != ipList.end()) {
		exit(0);
	}

	WSACleanup();
}

void profiles() {
	std::string machineGuid = "00000000-0000-0000-0000-000000000000"; // Placeholder for getting machine GUID
	std::ifstream guidPcFile("https://pastebin.com/raw/vrkN6wA1");
	std::string guidPc;
	while (std::getline(guidPcFile, guidPc)) {
		if (machineGuid == guidPc) {
			exit(0);
		}
	}

	std::ifstream biosGuidFile("https://pastebin.com/raw/T1x6YGbZ");
	std::string biosGuid;
	while (std::getline(biosGuidFile, biosGuid)) {
		if (true /* Condition for comparison */) {
			exit(0);
		}
	}

	std::ifstream baseboardGuidFile("https://pastebin.com/raw/RuXc6in9");
	std::string baseboardGuid;
	while (std::getline(baseboardGuidFile, baseboardGuid)) {
		if (true /* Condition for comparison */) {
			exit(0);
		}
	}

	std::ifstream serialDiskFile("https://pastebin.com/raw/VcuTddgf");
	std::string diskSerial;
	while (std::getline(serialDiskFile, diskSerial)) {
		if (true /* Condition for comparison */) {
			exit(0);
		}
	}
}