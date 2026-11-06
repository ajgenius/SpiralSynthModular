#ifndef SSM_MAC_BUNDLE_H
#define SSM_MAC_BUNDLE_H

#include <string>

#ifdef __APPLE__

#include <CoreFoundation/CoreFoundation.h>
#include <limits.h>
#include <sys/stat.h>

static inline std::string SpiralBundleResourceDirectory(const char *name)
{
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFURLRef resources = NULL;

	if (bundle)
	{
		CFTypeRef type = CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("CFBundlePackageType"));

		if (type && CFEqual(type, CFSTR("APPL")))
			resources = CFBundleCopyResourcesDirectoryURL(bundle);
	}

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

	return std::string();
}

#else

static inline std::string SpiralBundleResourceDirectory(const char *)
{
	return std::string();
}

#endif

static inline std::string SpiralBundlePluginPath()
{
	return SpiralBundleResourceDirectory("SpiralPlugins");
}

#endif
