#pragma once

// This file centralizes the configuration management (save files, opts.ini)
//
// Copyright (C) 2017 Greg Hedger, Ray Haleblian

#include <string>
#include <vector>
#include "ini.h"

#ifdef GALA
#define USER_DIRECTORY_NAME "/.config/linapple/"
#define DEFAULT_DSK "default.dsk"
#else
#define USER_DIRECTORY_NAME "/.linapple/"
#define DEFAULT_DSK "Master.dsk"
#endif
#define REGISTRY_NAME "linapple.conf"
#define CONF_DIRECTORY_NAME "/conf/"
#define SAVED_DIRECTORY_NAME "/saved/"
#define FTP_DIRECTORY_NAME "/ftp/"
#ifdef RESOURCE_INIT_DIR
  #define INSTALL_DIRECTORY_NAME RESOURCE_INIT_DIR "/"
#else
  #define INSTALL_DIRECTORY_NAME "/etc/linapple/"
#endif
#define MAX_FILENAME_LENGTH 255

class Config
{
	public:
		// Constructor/Destructor
		Config();
		~Config();

		bool ValidateUserDirectory();
		bool CopyFile(std::string source, std::string dest);
  std::string Get(std::string key);
  bool GetBoolean(std::string key);
  float GetFloat(std::string key);
  int GetInteger(std::string key);
		std::string GetUserFilePath();
		std::string GetRegistryPath();
		void SetRegistryPath(std::string path);
	protected:
		std::string FindFile(std::string filename, std::vector<std::string> paths);
		std::string FindDefaultDsk();
		std::string FindRegistryPath();
		std::string GetHomePath();
		std::string GetInstallPath();
		void Read();
		void Dump();
	private:
  std::map<const char*, bool> m_b;
std::map<const char*, float> m_f;
					     std::map<const char*, int> m_i;
std::map<const char*, std::string> m_s;
  std::string m_optsFilePath;
		std::string m_regFilePath;
		std::vector<std::string> m_regSearchPaths;  // aka /etc et al
		std::vector<std::string> m_resSearchPaths;  // aka /usr/share et al
		INIReader *conf;
};
