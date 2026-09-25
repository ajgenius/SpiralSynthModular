#ifndef SSM_MAC_BUNDLE_H
#define SSM_MAC_BUNDLE_H

#include <string>
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <limits.h>
#include <sys/stat.h>
#endif

// Return an existing resource directory, or an empty path outside a bundle.
static inline std::string SSMBundleResourceDirectory(const char *name)
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
            std::string directory = std::string(reinterpret_cast<const char *>(path)) + "/" + name;
            struct stat info;
            if (stat(directory.c_str(), &info) == 0 && S_ISDIR(info.st_mode))
                return directory;
        }
    }
#endif
    return std::string();
}

// Leave the configured Unix plugin directory in effect outside a bundle.
static inline std::string SSMBundlePluginPath()
{
    return SSMBundleResourceDirectory("SpiralPlugins");
}

#endif
