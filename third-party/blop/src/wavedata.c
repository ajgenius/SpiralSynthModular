#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <ladspa.h>
#include <config.h>
#include "wavedata.h"

#ifndef WAVEDATA_SUBDIR
#warning *** No wavedata subdir given, using default 'blop_files'
#define WAVEDATA_SUBDIR "blop_files"
#endif

int
wavedata_load (Wavedata * w,
               const char * wdat_descriptor_name,
               unsigned long sample_rate)
{
	const char * subdir = WAVEDATA_SUBDIR;
	char * ladspa_path;
	const char * start;
	const char * end;
	int extra;
	size_t subdirlen = strlen(WAVEDATA_SUBDIR);
	size_t length;
	size_t pathlen;
	char * path;
	char * filename;
	DIR * dp;
	struct dirent * ep;
	struct stat sb;
	void * handle;
	int (* desc_func)(Wavedata *, unsigned long);
	int retval = - 1;

/* Get LADPSA_PATH, if available */
	ladspa_path = getenv("LADSPA_PATH");
	if (!ladspa_path)
		ladspa_path = "/usr/lib/ladspa:/usr/local/lib/ladspa";

	start = ladspa_path;
	while (*start != '\0')
	{
		while (*start == ':')
			start++;

		end = start;
		while (*end != ':' && *end != '\0')
			end++;

		if (end - start > 0)
		{
			extra = (*(end - 1) == '/') ? 0 : 1;
			path = (char *) malloc (end - start + extra + subdirlen + 1 + 1);
			if (path)
			{
				strncpy (path, start, end - start);
				if (extra == 1)
					path[end - start] = '/';

				path[end - start + extra] = '\0';

				if (subdirlen > 0) {
					strncat(path, subdir, subdirlen);
					path[end - start + extra + subdirlen] = '/';
					path[end - start + extra + subdirlen + 1] = '\0';
				} else {
					path[end - start + extra + subdirlen] = '\0';
				}

				dp = opendir (path);
				if (dp)
				{
					pathlen = strlen (path);
					while ((ep = readdir (dp)))
					{
					/* Stat file to get type */
						length = pathlen + strlen (ep->d_name);
						filename = (char *) malloc (length + 1);
						if (filename)
						{
							strncpy (filename, path, pathlen);

							filename[pathlen] = '\0';
							filename = strncat (filename, ep->d_name, strlen (ep->d_name));
							filename[length] = '\0';

							if (!stat (filename, &sb))
							{
							/* We only want regular files */
								if (S_ISREG (sb.st_mode)) {
								/* Whew. Now see if we've got the right dll */
									handle = dlopen (filename, RTLD_NOW);

									if (handle) {
										desc_func = dlsym (handle, wdat_descriptor_name);

										if (desc_func) {
											free (filename);
											free (path);
											retval = desc_func (w, sample_rate);
											w->data_handle = handle;
											return retval;
										}
									}
								}
							}
							free (filename);
						}
					}
					closedir (dp);
				}
				free (path);
			}
		}
		start = end;
	}
	return retval;
}

/*
 * Close loaded wavedata
 */
void
wavedata_unload (Wavedata * w)
{
	dlclose (w->data_handle);
}
