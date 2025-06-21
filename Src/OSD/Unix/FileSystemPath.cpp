/**
 ** Supermodel
 ** A Sega Model 3 Arcade Emulator.
 ** Copyright 2003-2022 The Supermodel Team
 **
 ** This file is part of Supermodel.
 **
 ** Supermodel is free software: you can redistribute it and/or modify it under
 ** the terms of the GNU General Public License as published by the Free
 ** Software Foundation, either version 3 of the License, or (at your option)
 ** any later version.
 **
 ** Supermodel is distributed in the hope that it will be useful, but WITHOUT
 ** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 ** FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 ** more details.
 **
 ** You should have received a copy of the GNU General Public License along
 ** with Supermodel.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "FileSystemPath.h"
#include "Util/Format.h"
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

namespace FileSystemPath
{
    // Checks if a directory exists (returns true if exists, false if it doesn't)
    bool PathExists(std::string fileSystemPath)
    {
        struct stat pathInfo;
        return stat(fileSystemPath.c_str(), &pathInfo) == 0 && S_ISDIR(pathInfo.st_mode);
    }

    // If directory doesn't exist, create it
    int MakeDir(std::string dir)
    {
        if (!PathExists(dir))
        {
            return mkdir(dir.c_str(), 0775);
        }

        return 0;
    }

    std::string GetExePath()
    {
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        return std::string(result, (count > 0) ? count : 0);
    }

    std::string GetExeDir()
    {
        std::string exeDir = std::filesystem::path(GetExePath()).parent_path().u8string();
        return exeDir;
    }

    std::string GetGameXmlPath()
    {
        std::string defaultPath = Util::Format() << FileSystemPath::GetPath(FileSystemPath::Config) << "Games.xml";
        std::string finalPath = defaultPath;
        if (!std::filesystem::exists(defaultPath))
        {
            // Fall back to Games xml bundled with exe when unavailable in user home
            std::string bundledPath = std::filesystem::path(GetExeDir()) / "Config" / "Games.xml";
            if (std::filesystem::exists(bundledPath))
            {
                finalPath = bundledPath;
            }
        }
        return finalPath;
    }

    // Generates a path to be used by Supermodel files
    std::string GetPath(PathType pathType)
    {
        std::string finalPath;
        std::string homePath;
        std::string strPathType;
        struct passwd *pwd = getpwuid(getuid());

        // Resolve pathType to string for later use
        switch (pathType)
        {
        case Analysis:
            strPathType = "Analysis";
            break;
        case Config:
            strPathType = "Config";
            break;
        case Log:
            strPathType = "Log";
            break;
        case NVRAM:
            strPathType = "NVRAM";
            break;
        case Saves:
            strPathType = "Saves";
            break;
        case Screenshots:
            strPathType = "Screenshots";
            break;
        case Assets:
            strPathType = "Assets";
            break;
        }

        // Get user's HOME directory
        if (pwd)
        {
            homePath = pwd->pw_dir;
        }
        else
        {
            const char *envHome = getenv("HOME");
            homePath = envHome == NULL ? std::string() : envHome;
        }

        auto p1Crosshair = std::filesystem::path(GetExeDir()) / "Assets" / "p1crosshair.bmp";

        // Fall back to assets bundle with exe when unavailable in user Home
        if ((!FileSystemPath::PathExists("Assets") || homePath.empty()) &&
            strPathType == "Assets" && std::filesystem::exists(p1Crosshair))
        {
            auto fPath = std::filesystem::path(GetExeDir()) / strPathType;
            finalPath = fPath;
            FileSystemPath::MakeDir(finalPath);
            finalPath = Util::Format() << finalPath << "/";
            return finalPath;
        }

        // If Config path exists in current directory or the user doesn't have a HOME directory use current directory
        if (FileSystemPath::PathExists("Config") || homePath.empty())
        {
            // Use current directory
            if (pathType != Screenshots && pathType != Log)
            {
                finalPath = strPathType;
                FileSystemPath::MakeDir(finalPath);
            }
        }
        // Check if $HOME/.supermodel-dojo exists
        else if (FileSystemPath::PathExists(Util::Format() << homePath << "/.supermodel-dojo"))
        {
            // Use $HOME/.supermodel-dojo
            finalPath = Util::Format() << homePath << "/.supermodel-dojo/" << strPathType;
            FileSystemPath::MakeDir(finalPath);
        }
        // On Linux one may want to follow the XDG base directory specs (https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html)
        else
        {
            // Use $HOME/.config/supermodel-dojo or $HOME/.local/share/supermodel-dojo depending on the file type
            if (pathType == Config)
            {
                finalPath = Util::Format() << homePath << "/.config/supermodel-dojo";
                if (!FileSystemPath::PathExists(finalPath))
                {
                    const char *envConfig = getenv("XDG_CONFIG_HOME");
                    std::string configPath = (envConfig == NULL ? std::string() : envConfig);
                    if (!configPath.empty())
                    {
                        finalPath = Util::Format() << configPath << "/supermodel-dojo";
                    }
                }
                FileSystemPath::MakeDir(finalPath);

                finalPath = Util::Format() << finalPath << "/Config";
                FileSystemPath::MakeDir(finalPath);
            }
            else
            {
                finalPath = Util::Format() << homePath << "/.local/share/supermodel-dojo";
                if (!FileSystemPath::PathExists(finalPath))
                {
                    const char *envData = getenv("XDG_DATA_HOME");
                    std::string dataPath = (envData == NULL ? std::string() : envData);
                    if (!dataPath.empty())
                    {
                        finalPath = Util::Format() << dataPath << "/supermodel-dojo";
                    }
                }
                FileSystemPath::MakeDir(finalPath);

                finalPath = Util::Format() << finalPath << "/" << strPathType;
                FileSystemPath::MakeDir(finalPath);
            }
        }

        if (!finalPath.empty())
        {
            finalPath = Util::Format() << finalPath << "/";
        }
        return finalPath;
    }
}
