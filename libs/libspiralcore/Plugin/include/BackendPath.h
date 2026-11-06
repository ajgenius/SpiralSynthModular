#ifndef SSM_BACKEND_PATH_H
#define SSM_BACKEND_PATH_H
#include <string>
namespace Spiral
{
namespace Core
{
	void LoadAudioBackends(const std::string &root);
	void LoadMidiBackends(const std::string &root);
}
} // namespace Spiral
#endif
