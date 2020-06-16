// Central class for managing user configuration files and locations
//
// Copyright (C) 2017 Greg Hedger

#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <pwd.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"

Config::Config() {
	m_b.clear();
	m_f.clear();
	m_i.clear();
	m_s.clear();
	m_i["append to printer file"] = 1;
	m_i["boot at startup"] = 1;
	m_i["clock enable"] = 4;
	m_i["computer emulation"] = 3;
	m_s["disk image 1"] = std::string("");
	m_s["disk image 2"] = std::string("");
	m_i["emulation speed"] = 10;
	m_i["enhance disk speed"] = 1;
	m_s["ftp local dir"] = std::string("");
	m_s["ftp server"] = std::string(
	    "ftp://ftp.apple.asimov.net/pub/apple_ii/images/games/");
	m_s["ftp serverhdd"] =
	    std::string("ftp://ftp.apple.asimov.net/pub/apple_ii/images/");
	m_s["ftp userpass"] = std::string("anonymous:my-mail@mail.com");
	m_i["fullscreen"] = 1;
	m_i["harddisk enable"] = 0;
	m_s["harddisk image 1"] = std::string("");
	m_s["harddisk image 2"] = std::string("");
	m_s["hdv starting directory"] = std::string("");
	m_i["joy0axis0"] = 0;
	m_i["joy0axis1"] = 1;
	m_i["joy0button1"] = 0;
	m_i["joy0button2"] = 1;
	m_i["joy0index"] = 0;
	m_i["joy1axis0"] = 0;
	m_i["joy1axis1"] = 1;
	m_i["joy1button1"] = 0;
	m_i["joy1button2"] = 1;
	m_i["joy1index"] = 1;
	m_i["joyexitbutton0"] = 8;
	m_i["joyexitbutton1"] = 9;
	m_i["joyexitenable"] = 1;
	m_i["joystick 0"] = 3;
	m_i["joystick 1"] = 0;
	m_i["keyboard rocker switch"] = 0;
	m_i["keyboard type"] = 0;
	m_i["mouse in slot 4"] = 0;
	m_s["parallel printer filename"] = std::string("");
	m_i["printer idle limit"] = 10;
	m_s["save state directory"] = std::string("");
	m_s["save state filename"] = std::string("");
	m_i["save state on exit"] = 0;
	m_i["show leds"] = 1;
	m_i["serial port"] = 0;
	m_i["singlethreaded"] = 0;
	m_i["slot 6 autoload"] = 0;
	m_s["slot 6 directory"] = std::string("");
	m_i["sound emulation"] = 1;
	m_i["soundcard type"] = 2;
	m_i["video emulation"] = 1;

	m_optsFilePath = GetHomePath() + std::string(USER_DIRECTORY_NAME);

	m_regSearchPaths.clear();
	m_regSearchPaths.push_back(m_optsFilePath);
	// FIXME can this be described relative to the binary?
	m_regSearchPaths.push_back("/usr/local/etc/linapple");
	m_regSearchPaths.push_back(INSTALL_DIRECTORY_NAME);

	m_resSearchPaths.clear();
	m_resSearchPaths.push_back(m_optsFilePath);
	// FIXME can this be described relative to the binary?
	m_resSearchPaths.push_back("/usr/local/share/linapple");
	m_resSearchPaths.push_back("/usr/share/linapple");

	m_regFilePath = FindRegistryPath();

	if (m_regFilePath.size())
		std::cout << "configuration file: " << m_regFilePath
			  << std::endl;
	else
		std::cout << "configuration file not found!" << std::endl;

	conf = NULL;
	Read();
	Dump();
}

Config::~Config() {
	m_regSearchPaths.clear();
	m_resSearchPaths.clear();
	if (conf)
		delete (conf);
}

std::string Config::GetUserFilePath() { return m_optsFilePath; }

std::string Config::FindDefaultDsk() {
	return FindFile(std::string(DEFAULT_DSK), m_resSearchPaths);
}

std::string Config::FindRegistryPath() {
	return FindFile(std::string(REGISTRY_NAME), m_regSearchPaths);
}

std::string Config::FindFile(std::string filename,
			     std::vector<std::string> paths) {
	std::string match;
	std::vector<std::string>::iterator it;

	match.clear();
	for (it = paths.begin(); it != paths.end(); it++) {
		std::string path = *it + "/" + filename;
		std::cout << "checking: " << path << std::endl;
		std::ifstream ifs(path.c_str());
		if (ifs) {
			// A non-null stream means a file is there.
			match = path;
			std::cout << "found: " << match << std::endl;
			break;
		}
	}
	return match;
}

std::string Config::GetRegistryPath() { return m_regFilePath; }

void Config::SetRegistryPath(std::string path) { m_regFilePath = path; }

// Simple POSIX file copy
bool Config::CopyFile(std::string srcFile, std::string destFile) {
	const int bufSize = 1024;
	bool bRet = true;
	char buf[bufSize];
	size_t size;

	std::cout << "Copying '" << srcFile.c_str() << "' to '"
		  << destFile.c_str() << "'" << std::endl;

	// Attempt to open files
	int source = open(srcFile.c_str(), O_RDONLY, 0);
	int dest =
	    open(destFile.c_str(), O_WRONLY | O_CREAT /*| O_TRUNC*/, 0644);

	if (source && dest) {
		// Copy
		while ((size = read(source, buf, bufSize)) > 0) {
			if (0 >= write(dest, buf, size)) {
				// Handle error;
				std::cout << "Error writing '"
					  << destFile.c_str() << "' (" << size
					  << ")" << std::endl;
				std::cout << "Source file: " << srcFile.c_str()
					  << std::endl;
				bRet = false;
				break;
			}
		}

		// Close files
		if (source) {
			close(source);
		}
		if (dest) {
			close(dest);
		}
	} else {
		std::cout << "Error copying '" << srcFile.c_str() << "' to '"
			  << destFile.c_str() << "'" << std::endl;
		bRet = false;
	}
	return bRet;
}

// ValidateUserDirectory
// Checks for presence of user directory structure for configuration files
bool Config::ValidateUserDirectory() {

// GPH TOOD: Revisit with more elegant solution.
// Looks like there's an official way to copy all files in a directory
// for c++17 using filesystem::, but I just want something that's
// going to work.
#ifndef GALA
	static const char *files[] = {"Master.dsk", "linapple.conf", ""};
#endif
	bool bResult = false;
	struct stat buffer;
	std::string userDir = m_optsFilePath;
	std::string installDir = GetInstallPath();

	// Check that the entire subtree exists
	bResult = (stat(userDir.c_str(), &buffer) == 0);
	bResult &=
	    (stat((userDir + CONF_DIRECTORY_NAME).c_str(), &buffer) == 0);
	bResult &=
	    (stat((userDir + SAVED_DIRECTORY_NAME).c_str(), &buffer) == 0);
	bResult &= (stat((userDir + FTP_DIRECTORY_NAME).c_str(), &buffer) == 0);
	bResult &= (stat(GetUserFilePath().c_str(), &buffer) == 0);
	if (!bResult) {
		// Directory is absent.  This means we need to create it and
		// copy over defaults from the install location.
		mkdir(userDir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |
					   S_IWGRP | S_IROTH | S_IWOTH);
		mkdir((userDir + CONF_DIRECTORY_NAME).c_str(),
		      S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |
			  S_IROTH | S_IWOTH);
		mkdir((userDir + SAVED_DIRECTORY_NAME).c_str(),
		      S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |
			  S_IROTH | S_IWOTH);
		mkdir((userDir + FTP_DIRECTORY_NAME).c_str(),
		      S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |
			  S_IROTH | S_IWOTH);
	}

#ifndef GALA
	std::cout << "Copying Files" << std::endl;
	for (unsigned int i = 0; *files[i]; i++) {
		std::string src = (GetInstallPath() + files[i]);
		std::string dest = (GetUserFilePath() + files[i]);
		if (stat(dest.c_str(), &buffer) == 0) {
			// It's already there.
			continue;
		}
		if (!(stat(src.c_str(), &buffer) == 0)) {
			std::cout << "Could not stat " << src << "."
				  << std::endl;
			std::cout << "Please ensure " << GetInstallPath()
				  << " exists and contains the linapple "
				     "resource files."
				  << std::endl;
			throw std::runtime_error(
			    "could not copy resource files");
		}
		CopyFile(src, dest);
	}
#endif

	return bResult;
}

std::string Config::GetInstallPath() { return INSTALL_DIRECTORY_NAME; }

std::string Config::GetHomePath() {
	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;
	std::string path = homedir;
	return path;
}

void Config::Read() {
	if (!m_regFilePath.size())
		return;
	if (conf)
		delete (conf);
	conf = new INIReader(m_regFilePath);
}

void Config::Dump() {
	// TODO actually dump all keys
	std::cout << GetBoolean("boot on startup") << std::endl;
	std::cout << GetInteger("computer emulation") << std::endl;
	std::cout << GetBoolean("fullscreen") << std::endl;
}

bool Config::GetBoolean(std::string key) {
	return conf->GetInteger("", key.c_str(), m_b[key.c_str()]);
}

float Config::GetFloat(std::string key) {
	return conf->GetFloat("", key.c_str(), m_f[key.c_str()]);
}

int Config::GetInteger(std::string key) {
	return conf->GetInteger("", key.c_str(), m_i[key.c_str()]);
}

std::string Config::Get(std::string key) {
	return std::string(conf->Get("", key.c_str(), m_s[key.c_str()]));
}
