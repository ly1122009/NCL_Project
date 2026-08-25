#include <stdio.h>
#include <string.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    const char path_device[] = "/dev/video0";
    int ret = 0;

    int fd = open(path_device, O_RDWR);
    if (fd < 0)
    {
        printf("ERROR: Open failed\n");
        return 1;
    }
    printf("Open done!\n");
    
    struct v4l2_capability cap;
    memset(&cap, 0, sizeof(struct v4l2_capability));
    
    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret)
    {
        printf("ERROR: VIDIOC_QUERYCAP failed: %d\n",ret);
        return 1;
    }

    if (!(cap.device_caps & V4L2_CAP_DEVICE_CAPS))
    {
        printf("(!(cap.device_caps & V4L2_CAP_DEVICE_CAPS))\n");
        return 1;        
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        printf("ERROR:!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)\n");
        return 1;        
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        printf("ERROR:!(cap.capabilities & V4L2_CAP_STREAMING)\n");
        return 1;        
    }

    if (!(cap.device_caps & V4L2_CAP_VIDEO_CAPTURE))
    {
        printf("ERROR:!(cap.device_caps & V4L2_CAP_VIDEO_CAPTURE)\n");
        return 1;        
    }

    if (!(cap.device_caps & V4L2_CAP_STREAMING))
    {
        printf("ERROR:!(cap.device_caps & V4L2_CAP_STREAMING)\n");
        return 1;        
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(struct v4l2_format));
    
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(struct v4l2_requestbuffers));
    
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(struct v4l2_buffer));



    printf("Hello\n");

    return 0;
}
