// The backend search path. SSM_BACKEND_PATH, a colon list, overrides the
// root the host would use; LoadAudioBackends and LoadMidiBackends walk it
// with the audio and midi kinds. Carried from the private tree, where the
// same two calls sit on the settings-less path.
#ifndef SSM_BACKEND_PATH_H
#define SSM_BACKEND_PATH_H
#include <string>
namespace Spiral
{
namespace Core
{
	void LoadAudioBackends(const std::string &root);
	void LoadMidiBackends(const std::string &root);
} // namespace Core
} // namespace Spiral
#endif
