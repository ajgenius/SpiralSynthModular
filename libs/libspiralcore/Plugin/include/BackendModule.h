#ifndef SSM_BACKEND_MODULE_H
#define SSM_BACKEND_MODULE_H
namespace Spiral
{
namespace Core
{
	struct BackendDescriptor
	{
		unsigned ABI;
		const char *Kind, *Name;
		void *(*Create)(void *context);
		void (*Destroy)(void *instance);
	};
}
} // namespace Spiral
#endif
