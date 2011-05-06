
namespace android {
    class Test {
        public:
                static const sp<ISurface>& getISurface(const sp<Surface>& s) {
                            return s->getISurface();
                }
    };
};

#define SURFACE_CREATE(client,surface_ctrl,android_surface, android_isurface, win_width, win_height) \
{\
    client = new SurfaceComposerClient();\
    surface_ctrl = client->createSurface(getpid(), 0, win_width, win_height, PIXEL_FORMAT_RGB_565, ISurfaceComposer::ePushBuffers);\
    android_surface = surface_ctrl->getSurface();\
    android_isurface = Test::getISurface(android_surface);\
\
    client->openTransaction();\
    surface_ctrl->setPosition(0, 0);\
    client->closeTransaction();\
\
    client->openTransaction();\
    surface_ctrl->setSize(win_width, win_height);\
    client->closeTransaction();\
\
    client->openTransaction();\
    surface_ctrl->setLayer(0x100000);\
    client->closeTransaction();\
\
}\


