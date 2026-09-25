#ifndef SSM_MAC_BUNDLE_H
#define SSM_MAC_BUNDLE_H

#include <string>
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <limits.h>
#include <sys/stat.h>
#endif

// Return a bundle-local plugin path, or leave the Unix default in effect.
static std::string SSMBundlePluginPath()
{
#ifdef __APPLE__
    CFBundleRef bundle = CFBundleGetMainBundle();
    CFTypeRef type = bundle ? CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("CFBundlePackageType")) : NULL;
    if (!type || !CFEqual(type, CFSTR("APPL")))
        return std::string();
    CFURLRef resources = CFBundleCopyResourcesDirectoryURL(bundle);
    if (resources)
    {
        UInt8 path[PATH_MAX];
        Boolean valid = CFURLGetFileSystemRepresentation(resources, true, path, sizeof(path));
        CFRelease(resources);
        if (valid)
        {
            std::string plugins = std::string(reinterpret_cast<const char *>(path)) + "/SpiralPlugins";
            struct stat info;
            if (stat(plugins.c_str(), &info) == 0 && S_ISDIR(info.st_mode))
                return plugins;
        }
    }
#endif
    return std::string();
}

#endif
