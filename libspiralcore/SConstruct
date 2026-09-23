Target       = "libspiralcore.so"
Install   	 = "/usr/local/lib"
IncInstall	 = "/usr/local/include/spiralcore"
LibPaths     = Split("/usr/local/lib")
IncludePaths = Split("./include /usr/local/include")
	  
Libs = Split("sndfile jack lo m")

Source = Split("src/ChannelHandler.cpp \
				src/JackClient.cpp \
				src/Sample.cpp \
				src/AsyncSampleLoader.cpp \
				src/Time.cpp \
				src/Event.cpp \
				src/Tuna.cpp \
				src/EventQueue.cpp \
				src/OSCServer.cpp \
				src/RingBuffer.cpp \
				src/Allocator.cpp \
				src/CommandRingBuffer.cpp \
				src/SearchPaths.cpp")					

Headers = Split("include/ChannelHandler.h \
				include/JackClient.h \
				include/Sample.h \
				include/AsyncSampleLoader.h \
				include/Types.h \
				include/Trace.h \
				include/NoteTable.h \
				include/Time.h \
				include/Event.h \
				include/Tuna.h \
				include/EventQueue.h \
				include/OSCServer.h \
				include/RingBuffer.h \
				include/Allocator.h \
				include/CommandRingBuffer.h \
				include/SearchPaths.h")					

env = Environment(CCFLAGS = '-pipe -Wall -O3 -ggdb -ffast-math -Wno-unused  -fPIC')
env.SharedLibrary(source = Source, target = Target, LIBS=Libs, LIBPATH=LibPaths, CPPPATH=IncludePaths)

env.Install(Install, Target)
env.Alias('install', Install)

env.Install(IncInstall, Headers)
env.Alias('install', IncInstall)
