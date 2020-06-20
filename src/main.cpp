/*
Gala : An Apple //e (et al) emulator

Adapted from LinApple:
Copyright (C) 2015, Mark Ormond
Copyright (C) 2012, Krez Beotiger
Adapted from Applewin:
Copyright (C) 1994-1996, Michael O'Brien
Copyright (C) 1999-2001, Oliver Schmidt
Copyright (C) 2002-2005, Tom Charlesworth
Copyright (C) 2006-2007, Tom Charlesworth, Michael Pohoreski

Gala is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Gala is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with AppleWin; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Description: main() and a hodgepodge of functions not factored
 * into other modules.
 *
 * Author: Various
 *
 * History:
 * 2019: Significant rework of OS-facing internals and changes to UX.
 *       Geared towards future use on Raspberry Pi & RetroPie.
 *       Core still old AppleWin. "Gala".
 * 2018: Improvements ported over from the GitHub Linapple team.
 * 2015: LinApple was adapted in OCT 2015, as "linapple-pie",
 *       for use with Retropie, by Mark Ormond.
 * 2012: "LinApple", an AppleWin adaptation for SDL and POSIX (l)
 *       by beom beotiger, Nov-Dec 2007, krez beotiger March 2012 AD.
 */

#include <curl/curl.h>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include "stdafx.h"

#include "main.h"
#include "MouseInterface.h"
#include "cli.h"

#define CONFIG_FILE "gala.conf"
#define STARTUP_DSK "startup.dsk"
#define USER_CONFIG_DIR ".config/gala"
#define SDL_USEREVENT_RESTART 1
#define FTP_ANONYMOUS_LOGIN "anonymous:mymail@hotmail.com"

typedef std::vector<std::string> stringlist_t;

TCHAR *g_pAppTitle = TITLE_APPLE_2E_ENHANCED;
eApple2Type g_Apple2Type = A2TYPE_APPLE2EEHANCED;

int opt;
bool disablecursor = false;
BOOL behind = 0;	    // Redundant
DWORD cumulativecycles = 0; // Wraps after ~1hr 9mins
DWORD cyclenum = 0;	    // Used by SpkrToggle() for non-wave sound
DWORD emulmsec = 0;
static DWORD emulmsec_frac = 0;
bool g_bFullSpeed = false;
bool hddenabled = false;
static bool g_uMouseInSlot4 = false;

AppMode_e g_nAppMode = MODE_LOGO;

UINT g_ScreenWidth = SCREEN_WIDTH;
UINT g_ScreenHeight = SCREEN_HEIGHT;

DWORD needsprecision = 0; // Redundant
// TCHAR     g_sProgramDir[MAX_PATH] = TEXT("");
TCHAR g_sCurrentDir[MAX_PATH] =
	TEXT("");
TCHAR g_sHDDDir[MAX_PATH] =
	TEXT("");
TCHAR g_sSaveStateDir[MAX_PATH] = TEXT("");
TCHAR g_sParallelPrinterFile[MAX_PATH] =
	TEXT("Printer.txt");

TCHAR g_sFTPLocalDir[MAX_PATH] =
	TEXT(""); // FTP Local Dir, see linapple.conf for details
TCHAR g_sFTPServer[MAX_PATH] = TEXT("");    // full path to default FTP server
TCHAR g_sFTPServerHDD[MAX_PATH] = TEXT(""); // full path to default FTP server
TCHAR g_sFTPUserPass[512] = TEXT(FTP_ANONYMOUS_LOGIN); // full login line

bool g_bResetTiming = false; // Redundant
BOOL restart = 0;

// several parameters affecting the speed of emulated CPU
DWORD g_dwSpeed = SPEED_NORMAL; // Affected by Config dialog's speed slider bar
double g_fCurrentCLK6502 =
	CLK_6502;	   // Affected by Config dialog's speed slider bar
static double g_fMHz = 1.0; // Affected by Config dialog's speed slider bar

int g_nCpuCyclesFeedback = 0;
DWORD g_dwCyclesThisFrame = 0;

FILE *g_fh = NULL; // file for logging, let's use stderr instead?
bool g_bDisableDirectSound =
	false; // direct sound, use SDL Sound, or SDL_mixer???

CSuperSerialCard sg_SSC;
CMouseInterface sg_Mouse;

UINT g_Slot4 = CT_Mockingboard; // CT_Mockingboard or CT_MouseInterface

CURL *g_curl = NULL; // global easy curl resourse

void free_null(char *sz) {
	if (sz) {
		free(sz);
		sz = NULL;
	}
}

void ContinueExecution() {
	static BOOL pageflipping = 0; //?

	const double fUsecPerSec = 1.e6;

	const UINT nExecutionPeriodUsec = 1000; // 1.0ms
	const double fExecutionPeriodClks =
		g_fCurrentCLK6502 *
		((double)nExecutionPeriodUsec / fUsecPerSec);

	bool bScrollLock_FullSpeed =
		g_bScrollLock_FullSpeed; // g_uScrollLockToggle;

	g_bFullSpeed = ((g_dwSpeed == SPEED_MAX) || bScrollLock_FullSpeed ||
			(DiskIsSpinning() && enhancedisk && !Spkr_IsActive() &&
			 !MB_IsActive()));

	if (g_bFullSpeed) {
		// Don't call Spkr_Mute() - will get speaker clicks
		MB_Mute();
		SysClk_StopTimer();
		g_nCpuCyclesFeedback =
			0; // For the case when this is a big -ve number
	} else {
		// Don't call Spkr_Demute()
		MB_Demute();
		SysClk_StartTimerUsec(nExecutionPeriodUsec);
	}

	int nCyclesToExecute = (int)fExecutionPeriodClks + g_nCpuCyclesFeedback;
	if (nCyclesToExecute < 0)
		nCyclesToExecute = 0;

	DWORD dwExecutedCycles = CpuExecute(nCyclesToExecute);
	g_dwCyclesThisFrame += dwExecutedCycles;

	cyclenum = dwExecutedCycles;

	DiskUpdatePosition(dwExecutedCycles);
	JoyUpdatePosition();
	// the next call does not present	in current Applewin as on March
	// 2012??
	VideoUpdateVbl(g_dwCyclesThisFrame);

	SpkrUpdate(cyclenum);
	sg_SSC.CommUpdate(cyclenum);
	PrintUpdate(cyclenum);

	const DWORD CLKS_PER_MS = (DWORD)g_fCurrentCLK6502 / 1000;

	emulmsec_frac += dwExecutedCycles;
	if (emulmsec_frac > CLKS_PER_MS) {
		emulmsec += emulmsec_frac / CLKS_PER_MS;
		emulmsec_frac %= CLKS_PER_MS;
	}

	//
	// DETERMINE WHETHER THE SCREEN WAS UPDATED, THE DISK WAS SPINNING,
	// OR THE KEYBOARD I/O PORTS WERE BEING EXCESSIVELY QUERIED THIS
	// CLOCKTICK
	VideoCheckPage(0);
	BOOL screenupdated = VideoHasRefreshed();
	BOOL systemidle = 0; //(KeybGetNumQueries() > (clockgran << 2));	//  &&
			     //(!ranfinegrain);	// TO DO

	if (screenupdated)
		pageflipping = 3;

	//

	if (g_dwCyclesThisFrame >= dwClksPerFrame) {
		g_dwCyclesThisFrame -= dwClksPerFrame;

		if (g_nAppMode != MODE_LOGO) {
			VideoUpdateFlash();

			static BOOL anyupdates = 0;
			static DWORD lastcycles = 0;
			static BOOL lastupdates[2] = {0, 0};

			anyupdates |= screenupdated;

			//

			lastcycles = cumulativecycles;
			if ((!anyupdates) && (!lastupdates[0]) &&
			    (!lastupdates[1]) && VideoApparentlyDirty()) {
				VideoCheckPage(1);
				static DWORD lasttime = 0;
				DWORD currtime = GetTickCount();
				if ((!g_bFullSpeed) ||
				    (currtime - lasttime >=
				     (DWORD)((graphicsmode || !systemidle)
						     ? 100
						     : 25))) {
					VideoRefreshScreen();
					lasttime = currtime;
				}
				screenupdated = 1;
			}

			lastupdates[1] = lastupdates[0];
			lastupdates[0] = anyupdates;
			anyupdates = 0;

			if (pageflipping)
				pageflipping--;
		}

		MB_EndOfVideoFrame();
	}

	if (!g_bFullSpeed) {
		SysClk_WaitTimer();
	}
}

void SetCurrentCLK6502() {
	static DWORD dwPrevSpeed = (DWORD)-1;

	if (dwPrevSpeed == g_dwSpeed)
		return;

	dwPrevSpeed = g_dwSpeed;

	// SPEED_MIN    =  0 = 0.50 MHz
	// SPEED_NORMAL = 10 = 1.00 MHz
	//                20 = 2.00 MHz
	// SPEED_MAX-1  = 39 = 3.90 MHz
	// SPEED_MAX    = 40 = ???? MHz (run full-speed, /g_fCurrentCLK6502/ is
	// ignored)

	if (g_dwSpeed < SPEED_NORMAL)
		g_fMHz = 0.5 + (double)g_dwSpeed * 0.05;
	else
		g_fMHz = (double)g_dwSpeed / 10.0;

	g_fCurrentCLK6502 = CLK_6502 * g_fMHz;

	//
	// Now re-init modules that are dependent on /g_fCurrentCLK6502/
	//

	SpkrReinitialize();
	MB_Reinitialize();
}

void EnterMessageLoop() {
	SDL_Event event;

	while (true) {
		if (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT &&
			    event.key.keysym.sym != SDLK_F4)
				return;
			FrameDispatchMessage(&event);

			while ((g_nAppMode == MODE_RUNNING) ||
			       (g_nAppMode == MODE_STEPPING)) {
				if (SDL_PollEvent(&event)) {
					if (event.type == SDL_QUIT &&
					    event.key.keysym.sym != SDLK_F4)
						return;
					FrameDispatchMessage(&event);
				} else if (g_nAppMode == MODE_STEPPING) {
					DebugContinueStepping();
				} else {
					ContinueExecution();
					if (g_nAppMode != MODE_DEBUG) {
						if (joyexitenable) {
							CheckJoyExit();
							if (joyquitevent)
								return;
						}
						if (g_bFullSpeed)
							ContinueExecution();
					}
				}
			}
		} else {
			if (g_nAppMode == MODE_DEBUG)
				DebuggerUpdate();
			else if (g_nAppMode == MODE_LOGO ||
				 g_nAppMode == MODE_PAUSED)
				SDL_Delay(100); // Stop process hogging CPU
		}
	}
}

void LoadConfiguration(const cli_t &cli) {
	char *sz = NULL;
	DWORD dwTmp = 0; // temp var
	char *home = getenv("HOME");

	DWORD dwComputerType;
	LOAD(TEXT("Computer Emulation"), &dwComputerType);
	switch (dwComputerType) {
		// NB. No A2TYPE_APPLE2E
	case 0:
		g_Apple2Type = A2TYPE_APPLE2;
		break;
	case 1:
		g_Apple2Type = A2TYPE_APPLE2PLUS;
		break;
	case 2:
		g_Apple2Type = A2TYPE_APPLE2EEHANCED;
		break;
	default:
		g_Apple2Type = A2TYPE_APPLE2EEHANCED;
		break;
	}
	switch (g_Apple2Type) {
	case A2TYPE_APPLE2:
		g_pAppTitle = TITLE_APPLE_2;
		break;
	case A2TYPE_APPLE2PLUS:
		g_pAppTitle = TITLE_APPLE_2_PLUS;
		break;
	case A2TYPE_APPLE2E:
		g_pAppTitle = TITLE_APPLE_2E;
		break;
	case A2TYPE_APPLE2EEHANCED:
		g_pAppTitle = TITLE_APPLE_2E_ENHANCED;
		break;
	}

	// Game Controllers

	joytype[0] = 2;
	joytype[1] = 0;
	LOAD(TEXT("Joystick 0"), &joytype[0]);
	LOAD(TEXT("Joystick 1"), &joytype[1]);
	LOAD(TEXT("Joy0Index"), &joy1index);
	LOAD(TEXT("Joy1Index"), &joy2index);

	LOAD(TEXT("Joy0Button1"), &joy1button1);
	LOAD(TEXT("Joy0Button2"), &joy1button2);
	LOAD(TEXT("Joy1Button1"), &joy2button1);

	LOAD(TEXT("Joy0Axis0"), &joy1axis0);
	LOAD(TEXT("Joy0Axis1"), &joy1axis1);
	LOAD(TEXT("Joy1Axis0"), &joy2axis0);
	LOAD(TEXT("Joy1Axis1"), &joy2axis1);
	LOAD(TEXT("JoyExitEnable"), &joyexitenable);
	LOAD(TEXT("JoyExitButton0"), &joyexitbutton0);
	LOAD(TEXT("JoyExitButton1"), &joyexitbutton1);
	// for joysticks use default Y-,X-trims
	//   if(LOAD(TEXT(REGVALUE_PDL_XTRIM), &dwTmp))
	//       JoySetTrim((short)dwTmp, true);
	//   if(LOAD(TEXT(REGVALUE_PDL_YTRIM), &dwTmp))
	//       JoySetTrim((short)dwTmp, false);
	// we do not use this, scroll lock ever toggling full-speed???
	//   if(LOAD(TEXT(REGVALUE_SCROLLLOCK_TOGGLE), &dwTmp))
	// 	  g_uScrollLockToggle = dwTmp;

	LOAD(TEXT("Sound Emulation"), &soundtype);

	DWORD dwSerialPort;
	LOAD(TEXT("Serial Port"), &dwSerialPort);
	sg_SSC.SetSerialPort(dwSerialPort);

	LOAD(TEXT("Emulation Speed"), &g_dwSpeed);

	LOAD(TEXT("Enhance Disk Speed"), (DWORD *)&enhancedisk); //
	LOAD(TEXT("Video Emulation"), &videotype);

	LOAD(TEXT("Fullscreen"), &dwTmp); // load fullscreen flag
	if (cli.fullscreen && !strcmp(cli.fullscreen, "true"))
		fullscreen = true;
	else
		fullscreen = (BOOL)dwTmp;

		LOAD(TEXT("DisableCursor"), &dwTmp); // load Disable Cursor Flag
	disablecursor = (BOOL)dwTmp;

	dwTmp = 1;
	LOAD(TEXT(REGVALUE_SHOW_LEDS), &dwTmp); // load Show Leds flag
	g_ShowLeds = (BOOL)dwTmp;

	SetCurrentCLK6502(); // set up real speed

	if (LOAD(TEXT(REGVALUE_MOUSE_IN_SLOT4), &dwTmp))
		g_uMouseInSlot4 = dwTmp;
	g_Slot4 = g_uMouseInSlot4 ? CT_MouseInterface : CT_Mockingboard;

	if (LOAD(TEXT(REGVALUE_SOUNDCARD_TYPE), &dwTmp))
		MB_SetSoundcardType((eSOUNDCARDTYPE)dwTmp);

	if (LOAD(TEXT(REGVALUE_SAVE_STATE_ON_EXIT), &dwTmp))
		g_bSaveStateOnExit = dwTmp ? true : false;

	if (LOAD(TEXT(REGVALUE_HDD_ENABLED), &dwTmp))
		hddenabled = (bool)dwTmp; // after MemInitialize

	// Video

	if (RegLoadString(TEXT("Configuration"), TEXT("Monochrome Color"), 1,
			  &sz, 10)) {
		if (!sscanf(sz, "#%X", &monochrome))
			monochrome = 0xC0C0C0;
		free(sz);
		sz = NULL;
	}

	// Floppy drive image

	dwTmp = 0;
	LOAD(TEXT("Slot 6 Autoload"), &dwTmp);
	if (cli.imagefile) {
		DiskInsert(0, cli.imagefile, 0, 0);
		std::cerr << "[info ] Drive 1: " << cli.imagefile << std::endl;

	} else if (dwTmp) {
		if (RegLoadString(TEXT("Configuration"),
				  TEXT(REGVALUE_DISK_IMAGE1), 1, &sz,
				  MAX_PATH)) {
			DiskInsert(0, sz, 0, 0);
			std::cerr << "[info ] Drive 1: " << sz << std::endl;
			free_null(sz);
		}

	} else {
		struct stat statbuf;
		std::string filename(STARTUP_DSK);
		bool found = false;
		stringlist_t searchpaths = {
			".",
			std::string(getenv("HOME")) + "/" + USER_CONFIG_DIR,
			// TODO put ../share/linapple here...
			"/usr/local/share/gala", "/usr/share/gala"};
		std::string path;
		for (stringlist_t::iterator it = searchpaths.begin();
		     it != searchpaths.end(); it++) {
			path = *it + "/" + filename;
			if (!stat(path.c_str(), &statbuf)) {
				found = true;
				DiskInsert(0, path.c_str(), 0, 0);
				std::cerr << "[info ] Drive 1: " << path
					  << std::endl;
				break;
			}
		}

		if (!found)
			std::cerr << "[warn ] Startup disk not found"
				  << std::endl;
	}

	if (cli.imagefile2) {
		DiskInsert(1, cli.imagefile2, 0, 0);
		std::cerr << "[info ] Drive 2: " << cli.imagefile2 << std::endl;
	} else {
		if (RegLoadString(TEXT("Configuration"),
				  TEXT(REGVALUE_DISK_IMAGE2), 1, &sz,
				  MAX_PATH)) {
			DiskInsert(1, sz, 0, 0);
			std::cerr << "[info ] Drive 2: " << sz << std::endl;
			free_null(sz);
		}
	}

	// Hard drive image

	if (RegLoadString(TEXT("Configuration"), TEXT(REGVALUE_HDD_IMAGE1), 1,
			  &sz, MAX_PATH)) {
		HD_InsertDisk2(0, sz);
		free(sz);
		sz = NULL;
	}
	if (RegLoadString(TEXT("Configuration"), TEXT(REGVALUE_HDD_IMAGE2), 1,
			  &sz, MAX_PATH)) {
		HD_InsertDisk2(1, sz);
		free(sz);
		sz = NULL;
	}

	// Printer

	if (RegLoadString(TEXT("Configuration"),
			  TEXT(REGVALUE_PPRINTER_FILENAME), 1, &sz, MAX_PATH)) {
		if (strlen(sz) > 1)
			strncpy(g_sParallelPrinterFile, sz, MAX_PATH);
		free(sz);
		sz = NULL;
	}

	if (RegLoadString(TEXT("Configuration"), TEXT("Screen factor"), 1, &sz,
			  16)) {
		double scrFactor = atof(sz);
		if (scrFactor > 0.1) {
			g_ScreenWidth = UINT(g_ScreenWidth * scrFactor);
			g_ScreenHeight = UINT(g_ScreenHeight * scrFactor);
		}
		free(sz);
		sz = NULL;
	} else {
		dwTmp = 0;
		LOAD(TEXT("Screen Width"), &dwTmp);
		if (dwTmp > 0)
			g_ScreenWidth = dwTmp;
		dwTmp = 0;
		LOAD(TEXT("Screen Height"), &dwTmp);
		if (dwTmp > 0)
			g_ScreenHeight = dwTmp;
	}

	if (RegLoadString(TEXT("Configuration"),
			  TEXT(REGVALUE_SAVESTATE_FILENAME), 1, &sz,
			  MAX_PATH)) {
		Snapshot_SetFilename(sz);
		free(sz);
		sz = NULL;
	}

	RegLoadString(TEXT("Preferences"), REGVALUE_PREF_START_DIR, 1, &sz,
		      MAX_PATH);
	if (sz) {
		strcpy(g_sCurrentDir, sz);
		free_null(sz);
	}
	if (!strlen(g_sCurrentDir))
		strcpy(g_sCurrentDir, getenv("HOME"));

	RegLoadString(TEXT("Preferences"), REGVALUE_PREF_HDD_START_DIR, 1, &sz,
		      MAX_PATH);
	if (sz) {
		strcpy(g_sHDDDir, sz);
		free_null(sz);
	}

	if (strlen(g_sHDDDir) == 0) // something is wrong in dir name?
		strcpy(g_sHDDDir, home);

	RegLoadString(TEXT("Preferences"), REGVALUE_PREF_SAVESTATE_DIR, 1, &sz,
		      MAX_PATH);
	if (sz) {
		strcpy(g_sSaveStateDir, sz);
		free_null(sz);
	} else
		strcpy(g_sSaveStateDir, home);

	// FTP

	RegLoadString(TEXT("Preferences"), REGVALUE_FTP_DIR, 1, &sz, MAX_PATH);
	if (sz) {
		strcpy(g_sFTPServer, sz);
		free(sz);
		sz = NULL;
	}

	RegLoadString(TEXT("Preferences"), REGVALUE_FTP_HDD_DIR, 1, &sz,
		      MAX_PATH);
	if (sz) {
		strcpy(g_sFTPServerHDD, sz);
		free(sz);
		sz = NULL;
	}

	RegLoadString(TEXT("Preferences"), REGVALUE_FTP_LOCAL_DIR, 1, &sz,
		      MAX_PATH);
	if (sz) {
		strcpy(g_sFTPLocalDir, sz);
		free(sz);
		sz = NULL;
	}

	RegLoadString(TEXT("Preferences"), REGVALUE_FTP_USERPASS, 1, &sz, 512);
	if (sz) {
		strcpy(g_sFTPUserPass, sz);
		free(sz);
		sz = NULL;
	}

	// Boot

	dwTmp = 0;
	LOAD(TEXT("Boot at Startup"), &dwTmp);
	if (!dwTmp) {
		SDL_Event user_ev;
		user_ev.type = SDL_USEREVENT;
		user_ev.user.code = SDL_USEREVENT_RESTART;
		SDL_PushEvent(&user_ev);
	}
}

int main(int argc, char *argv[]) {
	cli_t cli;
	int error = parseCommandLine(argc, argv, &cli);
	if (error == ERROR_USAGE)
		return 0;

	// Open configuration file.

	registry = NULL;
	if (cli.conffile) {
		std::cerr << cli.conffile << std::endl;
		registry = fopen(cli.conffile, "r");
		if (!registry) {
			std::cerr << "[error] could not open "
				     "configuration file '"
				  << cli.conffile << "'." << std::endl;
			return 255;
		}
	} else {
		std::string conf;
		conf = CONFIG_FILE;
		registry = fopen(conf.c_str(), "r");
		if (!registry) {
			conf = std::string(getenv("HOME")) + "/" +
			       USER_CONFIG_DIR + "/" + CONFIG_FILE;
			registry = fopen(conf.c_str(), "r");
		}
		// This thing needs a registry file no matter what...
		if (!registry) {
			conf = "/tmp/gala.conf";
			registry = fopen(conf.c_str(), "w");
		}
		if (!registry)
			std::cerr << "[warn ] count not find or create a "
				     "configuration"
				  << std::endl;
		else
			std::cerr << "[info ] configuration: " << conf
				  << std::endl;
	}

	if (InitSDL())
		return 1; // init SDL subsystems, set icon

	curl_global_init(CURL_GLOBAL_DEFAULT);
	g_curl = curl_easy_init();
	if (!g_curl) {
		printf("Could not initialize CURL easy interface");
		return 1;
	}
	curl_easy_setopt(g_curl, CURLOPT_USERPWD, g_sFTPUserPass);
	/*bool bSysClkOK =*/SysClk_InitTimer();
	MemPreInitialize(); // Call before any of the slot devices are
			    // initialized
	ImageInitialize();
	DiskInitialize();
	CreateColorMixMap(); // For tv emulation g_nAppMode

	fullscreen = false;  // Frame.cpp
	
	do {
		restart = 0;
		g_nAppMode = MODE_LOGO;
		LoadConfiguration(cli);
		FrameCreateWindow();
		if (!DSInit())
			soundtype = SOUND_NONE; // Direct Sound and Stuff
		MB_Initialize();		// Mocking board
		SpkrInitialize();
		DebugInitialize();
		JoyInitialize();
		MemInitialize();
		HD_SetEnabled(hddenabled ? true : false);
		VideoInitialize();
		Snapshot_Startup(); // Do this after everything has been init'ed
		JoyReset();
		SetUsingCursor(0);
		if (disablecursor)
			SDL_ShowCursor(SDL_DISABLE);

		if (!fullscreen)
			SetNormalMode();
		else
			SetFullScreenMode();

		DrawFrameWindow();

		if (cli.benchmark)
			VideoBenchmark();
		else
			EnterMessageLoop();

		Snapshot_Shutdown();
		DebugDestroy();
		if (!restart) {
			DiskDestroy();
			ImageDestroy();
			HD_Cleanup();
		}
		PrintDestroy();
		sg_SSC.CommDestroy();
		CpuDestroy();
		MemDestroy();
		SpkrDestroy();
		VideoDestroy();
		MB_Destroy();
		MB_Reset();
		sg_Mouse.Uninitialize(); // Maybe restarting due to switching
					 // slot-4 card from mouse to MB
		JoyShutDown();		 // close opened (if any) joysticks
	} while (restart);

	DSUninit();
	SysClk_UninitTimer();
	if (g_fh) {
		fprintf(g_fh, "*** Logging ended\n\n");
		fclose(g_fh);
	}

	RiffFinishWriteFile();
	if (registry) {
		fclose(registry);
		registry = NULL;
	}

	SDL_Quit();
	curl_easy_cleanup(g_curl);
	curl_global_cleanup();
	return 0;
}