#include <ui/DisplayInfo.h>

namespace android {
    class Test {
        public:
                static const sp<ISurface>& getISurface(const sp<Surface>& s) {
                            return s->getISurface();
                }
    };
};

#define min(a,b) (a<b?a:b)
#define SURFACE_CREATE(client,surface_ctrl,android_surface, android_isurface, x, y, win_width, win_height) \
do {                                                                    \
    client = new SurfaceComposerClient();                               \
    android::DisplayInfo info;                                          \
    int w, h;                                                           \
                                                                        \
    client->getDisplayInfo(android::DisplayID(0), &info);               \
    /*w = min(win_width, info.w);*/                                     \
    /*h = min(win_height, info.h);*/                                    \
    w = win_width, h = win_height;                                      \
                                                                        \
    surface_ctrl = client->createSurface(getpid(), 0, w, h, PIXEL_FORMAT_RGB_565, ISurfaceComposer::ePushBuffers); \
    android_surface = surface_ctrl->getSurface();                       \
    android_isurface = Test::getISurface(android_surface);              \
                                                                        \
    client->openTransaction();                                          \
    surface_ctrl->setPosition(x, y);                                    \
    client->closeTransaction();                                         \
                                                                        \
    client->openTransaction();                                          \
    surface_ctrl->setSize(w, h);                                        \
    client->closeTransaction();                                         \
                                                                        \
    client->openTransaction();                                          \
    surface_ctrl->setLayer(0x100000);                                   \
    client->closeTransaction();                                         \
} while (0)


