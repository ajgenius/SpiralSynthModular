#ifndef SPIRALCORE_AUDIO_BACKEND_H
#define SPIRALCORE_AUDIO_BACKEND_H
#include <string>
#include <vector>
#include <map>
#include "AudioClient.h"
#include "BackendModule.h"
#include "PluginManager.h"
namespace Spiral
{
namespace Core
{
#define SPIRAL_AUDIO_PLUGIN_ABI 1
	class AudioBackendRegistry : public PluginKind
	{
	public:
		static AudioBackendRegistry *Get();
		static void PackUpAndGoHome();
		void Register(const BackendDescriptor *);
		unsigned LoadModules(const std::string &root);
		const BackendDescriptor *Find(const std::string &) const;
		std::vector<std::string> Names() const;
		AudioClient *Create(const std::string &name);
		void Destroy(const std::string &name, AudioClient *client);

		virtual const char *Name() const { return "audio"; }
		virtual const char *Subdirectory() const { return "audio"; }
		virtual const char *Suffix() const { return "_Audio"; }
		virtual const char *EntrySymbol() const { return "SpiralPlugin_GetAudioBackend"; }
		virtual bool Accept(void *entry, const std::string &path);
		virtual bool CanUnload() const { return clients.empty(); }
		virtual void Release(void *entry, const std::string &path);

	private:
		AudioBackendRegistry();
		~AudioBackendRegistry();
		std::vector<const BackendDescriptor *> entries;
		std::map<AudioClient *, const BackendDescriptor *> clients;
		static AudioBackendRegistry *m_Singleton;
		AudioBackendRegistry(const AudioBackendRegistry &);
		AudioBackendRegistry &operator=(const AudioBackendRegistry &);
	};
	const BackendDescriptor *DummyBackend();
}
} // namespace Spiral
extern "C"
{
	typedef const Spiral::Core::BackendDescriptor *(*SpiralPlugin_GetAudioBackendFn)();
}
#endif
