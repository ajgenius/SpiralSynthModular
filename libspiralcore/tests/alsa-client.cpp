// ALSA's null PCM exercises negotiation and nonblocking I/O without hardware.
#include "AlsaClient.h"
#include <cassert>
#include <vector>
using namespace spiralcore;
int main()
{
	AlsaClient *client = AlsaClient::Get();
	AudioClientOptions options;
	options.BufferSize = 128;
	for (unsigned mode = 0; mode < 3; ++mode)
	{
		options.InChannels = mode == 0 ? 0 : 2;
		options.OutChannels = mode == 1 ? 0 : 2;
		assert(client->Attach("null", options));
		std::vector<float> buffer(client->GetBufferSize() * 2, 0.25);
		for (unsigned n = 0; n < 10; ++n)
		{
			assert(client->WaitForCycle(10) == 1);
			AudioCycleTiming timing;
			assert(client->GetCycleTiming(timing) && timing.SampleRate > 0);
			assert(timing.Frame == n * client->GetBufferSize());
			if (options.InChannels) assert(client->Read(&buffer[0], client->GetBufferSize()));

			if (options.OutChannels) assert(client->Write(&buffer[0], client->GetBufferSize()));

		}

		client->Detach();
		assert(!client->IsAttached());
	}

	options.InChannels = 1;
	options.OutChannels = 2;
	assert(!client->Attach("null", options));
	AlsaClient::PackUpAndGoHome();
}
