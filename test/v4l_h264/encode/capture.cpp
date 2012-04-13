/*
 * Copyright (c) 2012 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This example is based on: http://linuxtv.org/downloads/v4l-dvb-apis/capture-example.html
 *  V4L2 video capture example
 *      This program is provided with the V4L2 API
 * see http://linuxtv.org/docs.php for more information
*/

#include <cstdlib> /* EXIT_FAILURE, EXIT_SUCCESS */
#include <string>
#include <cstring> /* strerror() */
#include <cassert>
#include <getopt.h> /* getopt_long() */
#include <fcntl.h> /* low-level i/o */
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <csignal>

#include <iostream>
#include <linux/videodev2.h>

using std::string;

#include "TCPSocketClient.h"


extern bool g_Force_P_Only;
extern bool g_ShowNumber;
extern bool g_LiveView;
char device_settings_buffer[255];
char *device_settings = NULL;
int  g_Debug = 0;
int  g_Numerator = 0;
int  g_FrameRate = 0;
TCPSocketClient *sock_ptr = NULL;
std::string ip_name = "localhost";
int   ip_port = 8888;
extern int g_PX;
extern int g_PY;
extern int win2_width;
extern int win2_height;




int   encoder_init(int width, int height);
int   encode_frame(unsigned char *inbuf);
void  encoder_close();

#define CLEAR(x) memset (&(x), 0, sizeof (x))

typedef enum {
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
} io_method;

struct buffer {
    void * start;
    size_t length;
};

static char * dev_name = NULL;
static io_method io = IO_METHOD_MMAP;
static int fd = -1;
struct buffer * buffers = NULL;
static unsigned int n_buffers = 0;
static unsigned int width = 176;
static unsigned int height = 144;
static unsigned int pixelformat = V4L2_PIX_FMT_YUYV;

static int time_to_quit = 0;
static void SignalHandler(int a_Signal)
{
    time_to_quit = 1;
    signal(SIGINT, SIG_DFL);
}

static void
    errno_exit (const char * s)
{
    std::cerr << s << " error " << errno << ", " << strerror(errno) << '\n';
    exit (EXIT_FAILURE);
}

static int xioctl (int fd, int request, void * arg)
{
    int r;
    do r = ioctl (fd, request, arg);
    while (-1 == r && EINTR == errno);
    return r;
}


static void
    process_image (const void * p, ssize_t size)
{
    const size_t src_frame_size = (width*height) + height*(width >> 1) + height*(width >> 1);
    if (size != src_frame_size){
        std::cerr << "wrong buffer size: " << size << "; expect: " << src_frame_size << '\n';
        return;
    }
    if (!encode_frame((unsigned char *)p))
        time_to_quit = 1;
}

static int
    read_frame (void)
{
    struct v4l2_buffer buf;
    unsigned int i;
    switch (io) {
    case IO_METHOD_READ:
        if (-1 == read (fd, buffers[0].start, buffers[0].length)) {
            switch (errno) {
            case EAGAIN:
                return 0;
            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
            default:
                errno_exit ("read");
            }
        }
        process_image (buffers[0].start, buffers[0].length);
        break;

    case IO_METHOD_MMAP:
        CLEAR (buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
            default:
                errno_exit ("VIDIOC_DQBUF");
            }
        }

        assert (buf.index < n_buffers);

        process_image (buffers[buf.index].start, buf.length);

        if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
            errno_exit ("VIDIOC_QBUF");

        break;

    case IO_METHOD_USERPTR:
        CLEAR (buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
            default:
                errno_exit ("VIDIOC_DQBUF");
            }
        }

        for (i = 0; i < n_buffers; ++i)
            if (buf.m.userptr == (unsigned long) buffers[i].start && buf.length == buffers[i].length)
                break;

        assert (i < n_buffers);
        process_image ((void *) buf.m.userptr, buf.length);
        if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
            errno_exit ("VIDIOC_QBUF");

        break;
    }

    return 1;
}

static void
    mainloop (void)
{
    while (!time_to_quit) {
        for (;;) {
            fd_set fds;
            struct timeval tv;
            int r;

            FD_ZERO (&fds);
            FD_SET (fd, &fds);

            /* Timeout. */
            tv.tv_sec = 5;
            tv.tv_usec = 0;

            r = select (fd + 1, &fds, NULL, NULL, &tv);

            if (-1 == r) {
                if (EINTR == errno)
                    continue;

                errno_exit ("select");
            }

            if (0 == r) {
                std::cerr << "select timeout\n";
                exit (EXIT_FAILURE);
            }

            if (read_frame ())
                break;

            /* EAGAIN - continue select loop. */
        }
    }
}

static void
    stop_capturing (void)
{
    enum v4l2_buf_type type;

    switch (io) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
            errno_exit ("VIDIOC_STREAMOFF");

        break;
    }
}

static void
    start_capturing (void)
{
    unsigned int i;
    enum v4l2_buf_type type;

    switch (io) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR (buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                errno_exit ("VIDIOC_QBUF");
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
            errno_exit ("VIDIOC_STREAMON");

        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR (buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            buf.index = i;
            buf.m.userptr = (unsigned long) buffers[i].start;
            buf.length = buffers[i].length;

            if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                errno_exit ("VIDIOC_QBUF");
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
            errno_exit ("VIDIOC_STREAMON");

        break;
    }
}

static void uninit_device (void)
{
    unsigned int i;

    switch (io) {
    case IO_METHOD_READ:
        free (buffers[0].start);
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i)
            if (-1 == munmap (buffers[i].start, buffers[i].length))
                errno_exit ("munmap");
        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < n_buffers; ++i)
            free (buffers[i].start);
        break;
    }

    free (buffers);
}

static void init_read (unsigned int buffer_size)
{
    buffers = (buffer*)calloc (1, sizeof (*buffers));

    if (!buffers) {
        std::cerr << "Out of memory\n";
        exit (EXIT_FAILURE);
    }

    buffers[0].length = buffer_size;
    buffers[0].start = malloc (buffer_size);

    if (!buffers[0].start) {
        std::cerr << "Out of memory\n";
        exit (EXIT_FAILURE);
    }
}

static void init_mmap (void)
{
    struct v4l2_requestbuffers req;

    CLEAR (req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            std::cerr << dev_name << " does not support "
                << "memory mapping\n";
            exit (EXIT_FAILURE);
        } else {
            errno_exit ("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        std::cerr << "Insufficient buffer memory on " << dev_name << '\n';
        exit (EXIT_FAILURE);
    }

    buffers = (buffer*)calloc (req.count, sizeof (*buffers));

    if (!buffers) {
        std::cerr << "Out of memory\n";
        exit (EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;
        CLEAR (buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;
        if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
            errno_exit ("VIDIOC_QUERYBUF");
        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =
            mmap (NULL /* start anywhere */,
            buf.length,
            PROT_READ | PROT_WRITE /* required */,
            MAP_SHARED /* recommended */,
            fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start)
            errno_exit ("mmap");
    }
}

static void init_userp (unsigned int buffer_size)
{
    struct v4l2_requestbuffers req;
    unsigned int page_size;
    page_size = getpagesize ();
    buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);
    CLEAR (req);
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;
    if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            std::cerr << dev_name << " does not support "
                << "user pointer i/o\n";
            exit (EXIT_FAILURE);
        } else {
            errno_exit ("VIDIOC_REQBUFS");
        }
    }
    buffers = (buffer*) calloc (4, sizeof (*buffers));
    if (!buffers) {
        std::cerr << "Out of memory\n";
        exit (EXIT_FAILURE);
    }
    for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
        buffers[n_buffers].length = buffer_size;
        buffers[n_buffers].start = memalign (/* boundary */ page_size,
            buffer_size);

        if (!buffers[n_buffers].start) {
            std::cerr << "Out of memory\n";
            exit (EXIT_FAILURE);
        }
    }
}

static void init_device (void)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;
    if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            std::cerr << dev_name << " is no V4L2 device\n";
            exit (EXIT_FAILURE);
        } else {
            errno_exit ("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        std::cerr << dev_name << " is no video capture device\n";
        exit (EXIT_FAILURE);
    }

    switch (io) {
    case IO_METHOD_READ:
        if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
            std::cerr << dev_name << " does not support read i/o\n";
            exit (EXIT_FAILURE);
        }

        break;
    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            std::cerr << dev_name << " does not support streaming i/o\n";
            exit (EXIT_FAILURE);
        }
        break;
    }
    /* Select video input, video standard and tune here. */
    CLEAR (cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */
        if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
                /* Cropping not supported. */
                break;
            default:
                /* Errors ignored. */
                break;
            }
        }
    } else {
        /* Errors ignored. */
    }
    struct v4l2_fmtdesc     fmtdesc;
    printf("video capture\n");
    for (int i = 0;; i++) {
        memset(&fmtdesc,0,sizeof(fmtdesc));
        fmtdesc.index = i;
        fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl (fd, VIDIOC_ENUM_FMT,&fmtdesc))
            break;
        printf("    VIDIOC_ENUM_FMT(%d,VIDEO_CAPTURE)\n",i);
        printf("pfmt: 0x%x %s\n",fmtdesc.pixelformat,fmtdesc.description);
    if (fmtdesc.pixelformat != 0x56595559) {
         printf("   => don't list not supported format\n");
         continue;
    }
        for (int k = 0;; k++) {
            struct v4l2_frmsizeenum frmsize;
            memset(&frmsize,0,sizeof(frmsize));
            frmsize.index = k;
            frmsize.pixel_format = fmtdesc.pixelformat;
            if (-1 == xioctl (fd, VIDIOC_ENUM_FRAMESIZES,&frmsize))
                break;
            if (frmsize.type== V4L2_FRMSIZE_TYPE_DISCRETE) {
                printf("       VIDIOC_ENUM_FRAMESIZES(%d,0x%x) %dx%d  @",k, frmsize.type, frmsize.discrete.width, frmsize.discrete.height);
                for (int l = 0;; l++) {
                    struct v4l2_frmivalenum frmrate;
                    memset(&frmrate, 0, sizeof(frmrate));
                    frmrate.index = l;
                    frmrate.pixel_format = fmtdesc.pixelformat;
                    frmrate.width = frmsize.discrete.width;
                    frmrate.height = frmsize.discrete.height;
                    if (-1 == xioctl (fd, VIDIOC_ENUM_FRAMEINTERVALS,&frmrate))
                        break;
                    if (frmrate.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                        printf(" %u/%u ", frmrate.discrete.numerator, frmrate.discrete.denominator);
                    }
                }
                printf("\n");
            }
        }
    }
    CLEAR (fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl (fd, VIDIOC_G_FMT, &fmt))
        errno_exit ("VIDIOC_G_FMT");
    printf("video: %dx%d; fourcc:0x%x\n", fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.pixelformat);
    if (fmt.fmt.pix.width != width || fmt.fmt.pix.height != height || (fmt.fmt.pix.pixelformat != pixelformat)){
        struct v4l2_pix_format def_format;
        memcpy(&def_format, &fmt.fmt.pix, sizeof(struct v4l2_pix_format));
        fmt.fmt.pix.width = width;
        fmt.fmt.pix.height = height;
        fmt.fmt.pix.pixelformat = pixelformat;
        if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt)){
            std::cerr << "failed to set resolution " << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height << '\n';
            errno_exit ("VIDIOC_S_FMT");
        }
    }
    if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
        errno_exit ("VIDIOC_S_FMT");

    CLEAR (fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl (fd, VIDIOC_G_FMT, &fmt))
        errno_exit ("VIDIOC_G_FMT");
    printf("video: %dx%d; fourcc:0x%x\n", fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.pixelformat);
  if (fmt.fmt.pix.width != width || fmt.fmt.pix.height != height || fmt.fmt.pix.pixelformat != pixelformat){
        errno_exit ("VIDIOC_S_FMT not set !");
  }

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;
    struct v4l2_streamparm capp;
    CLEAR(capp);
    capp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (g_FrameRate) {
        if (-1 == xioctl (fd, VIDIOC_G_PARM, &capp) == -1) {
            errno_exit ("VIDIOC_G_PARM");
        }
        printf("vidioc_s_parm called frate=%d/%d\n", capp.parm.capture.timeperframe.numerator, capp.parm.capture.timeperframe.denominator);
        capp.parm.capture.timeperframe.numerator = g_Numerator;
        capp.parm.capture.timeperframe.denominator = g_FrameRate;
        printf("vidioc_s_parm set: frate=%d/%d\n", capp.parm.capture.timeperframe.numerator, capp.parm.capture.timeperframe.denominator);
        if (-1 == xioctl (fd, VIDIOC_S_PARM, &capp) == -1) {
            errno_exit ("VIDIOC_S_PARM");
        }
        if (-1 == xioctl (fd, VIDIOC_G_PARM, &capp) == -1) {
            errno_exit ("VIDIOC_G_PARM");
        }
        if ((capp.parm.capture.timeperframe.numerator != g_Numerator) || (
        capp.parm.capture.timeperframe.denominator != g_FrameRate)) {
            errno_exit ("VIDIOC_S_PARM NOT SET");
        }
    } else {
        if (-1 == xioctl (fd, VIDIOC_G_PARM, &capp) == -1) {
            errno_exit ("VIDIOC_G_PARM");
        }
    }
    sprintf(device_settings_buffer,"%dx%d@%d/%d", width, height, capp.parm.capture.timeperframe.numerator, capp.parm.capture.timeperframe.denominator);
  device_settings = &device_settings_buffer[0];
  printf("INFO: %s\n", device_settings);


    switch (io) {
    case IO_METHOD_READ:
        init_read (fmt.fmt.pix.sizeimage);
        break;

    case IO_METHOD_MMAP:
        init_mmap ();
        break;

    case IO_METHOD_USERPTR:
        init_userp (fmt.fmt.pix.sizeimage);
        break;
    }
}

static void
    close_device (void)
{
    if (-1 == close (fd))
        errno_exit ("close");

    fd = -1;
}

static void open_device (void)
{
    struct stat st;

    if (-1 == stat (dev_name, &st)) {
        std::cerr << "Cannot identify '" << dev_name << "': " << errno << ", " << strerror(errno) << '\n';
        exit (EXIT_FAILURE);
    }

    if (!S_ISCHR (st.st_mode)) {
        std::cerr << dev_name << " is no device\n";
        exit (EXIT_FAILURE);
    }

    fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd) {
        std::cerr << "Cannot open '" << dev_name << "': " << errno << ", " << strerror(errno) << '\n';
        exit (EXIT_FAILURE);
    }
}

static void usage (std::ostream &o, int argc, char ** argv)
{
    o << "Usage: " << argv[0] << " [options]\n"
        "\n"
        "Options:\n"
        "-?, --help             Print this message\n"
        "-d, --device=NAME      Video device name ["<< dev_name<< "]\n"
        "-i, --ip=IP            Target ip [localhost]\n"
        "-p, --port=PORT        Target port [" << ip_port << "]\n"
        "-m, --mmap             Use memory mapped buffers\n"
        "-r, --read             Use read() calls\n"
        "-u, --userp            Use application allocated buffers\n"
        "-N, --no-number        Disable OSD\n"
        "-P, --progressive      Force P frames only\n"
        "-l, --liveview-off     Live View off\n"
        "-f, --framerate=FPS    Framerate [no limit]\n"
        "-w, --width=WIDTH      Window width [" << win2_width << "]\n"
        "-h, --height=HEIGHT    Window height [" << win2_height << "]\n"
        "-x, --posx=POS_X       X position [WM handles placement]\n"
        "-y, --posy=POS_Y       Y position [WM handles placement]\n"
        "-n, --numerator=NUM    Numerator [no limit]\n"
        "-W, --dev-width=WIDTH  Device width ["<< width << "]\n"
        "-H, --dev-height=HEIGHT  Device height [" << height << "]\n"
        "-D, --debug=LEVEL      Debug level [" << g_Debug << "]\n"
        "\n";
}

void InitSock()
{
    printf("Using: %s:%d\n", ip_name.c_str(), ip_port);
    try {
        sock_ptr = new TCPSocketClient(ip_name, ip_port);
    }
    catch (const std::exception& e)
    {
        printf("%s\n", e.what());
        exit(1);
    }
}


static const char short_options [] = "d:i:p:?mruNPlf:w:h:x:y:n:W:H:D:";

static const struct option
    long_options [] = {
        { "device",        required_argument, NULL, 'd' },
        { "ip",            required_argument, NULL, 'i' },
        { "port",          required_argument, NULL, 'p' },
        { "help",          no_argument,       NULL, '?' },
        { "mmap",          no_argument,       NULL, 'm' },
        { "read",          no_argument,       NULL, 'r' },
        { "userp",         no_argument,       NULL, 'u' },
        { "no-number",     no_argument,       NULL, 'N' },
        { "progressive",   no_argument,       NULL, 'P' },
        { "liveview-off",  no_argument,       NULL, 'l' },
        { "framerate",     required_argument, NULL, 'f' },
        { "width",         required_argument, NULL, 'w' },
        { "height",        required_argument, NULL, 'h' },
        { "posx",          required_argument, NULL, 'x' },
        { "posy",          required_argument, NULL, 'y' },
        { "numerator",     required_argument, NULL, 'n' },
        { "dev-width",     required_argument, NULL, 'W' },
        { "dev-height",    required_argument, NULL, 'H' },
        { "debug",         required_argument, NULL, 'D' },
        { 0, 0, 0, 0 }
};

int
    main (int argc, char ** argv)
{
    width = 640;
    height = 480;
    dev_name = (char*)"/dev/video0";

    for (;;) {
        int index;
        int c;

        c = getopt_long (argc, argv,
            short_options, long_options,
            &index);

        if (-1 == c)
            break;

        switch (c) {
        case 0: /* getopt_long() flag */
            break;

        case 'd':
            dev_name = optarg;
            break;
        case 'i':
            ip_name = optarg;
            break;
        case 'p':
            ip_port = atoi(optarg);
            break;
        case '?':
            usage (std::cout, argc, argv);
            exit (EXIT_SUCCESS);
        case 'm':
            io = IO_METHOD_MMAP;
            break;
        case 'r':
            io = IO_METHOD_READ;
            break;
        case 'u':
            io = IO_METHOD_USERPTR;
            break;
        case 'N':
            g_ShowNumber = false;
            break;
        case 'P':
            g_Force_P_Only = true;
            break;
        case 'l':
            g_LiveView = false;
            break;
        case 'f':
            g_FrameRate = atoi(optarg);
            if (!g_Numerator) g_Numerator = 1;
            break;
        case 'w':
            win2_width = atoi(optarg);
            break;
        case 'h':
            win2_height = atoi(optarg);
            break;
        case 'x':
            g_PX = atoi(optarg);
            break;
        case 'y':
            g_PY = atoi(optarg);
            break;
        case 'n':
            g_Numerator = atoi(optarg);
            break;
        case 'W':
            width = atoi(optarg);
            break;
        case 'H':
            height = atoi(optarg);
            break;
        case 'D':
            g_Debug = atoi(optarg);
            break;
        default:
            usage (std::cerr, argc, argv);
            exit (EXIT_FAILURE);
        }
    }
    if (g_Debug) {
        printf("Capture: %dx%d %d/%d\n", width, height, g_Numerator, g_FrameRate);
        printf("Win: %dx%d (%d,%d)\n", win2_width, win2_height, g_PX, g_PY);
    }

    if(signal(SIGINT, SignalHandler) == SIG_ERR){
        printf("signal() failed\n");
        time_to_quit = 1;
    }
    InitSock();
    open_device();
    pixelformat = V4L2_PIX_FMT_YUYV;
    init_device ();
    printf("negotiated frame resolution: %dx%d\n", width, height);

    if (!encoder_init(width, height)) {
        start_capturing ();
        mainloop ();
        stop_capturing ();
        encoder_close();
    } else {
        printf("Error: encoder init !\n");
    }
    uninit_device ();
    close_device ();
    delete sock_ptr;
    return 0;
}
