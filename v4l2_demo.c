#include <stdio.h>
#include <string.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define CLEAR(x) memset(&(x), 0 , sizeof(x))

int main(int argc, char **argv)
{
    const char path_device[] = "/dev/video0";
    int ret = 0;

    int fd = open(path_device, O_RDWR);
    if (fd < 0) {
        printf("ERROR: Open failed\n");
        return 1;
    }
    printf("Open done!\n");

    struct v4l2_capability cap;
    memset(&cap, 0, sizeof(struct v4l2_capability));

    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret) {
        printf("ERROR: VIDIOC_QUERYCAP failed: %d\n", ret);
        return 1;
    }

    printf("Driver:  %s \n", cap.driver);
    printf("Card:  %s \n", cap.card);
    printf("Bus info:  %s \n", cap.bus_info);

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        printf("ERROR:!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)\n");
        return 1;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        printf("ERROR:!(cap.capabilities & V4L2_CAP_STREAMING)\n");
        return 1;
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(struct v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 1920;
    fmt.fmt.pix.height = 1080;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    
    ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
    if (ret) {
        printf("ERROR: VIDIOC_S_FMT failed: %d\n", ret);
        return 1;
    }

    struct v4l2_requestbuffers reqbufs;
    memset(&reqbufs, 0, sizeof(struct v4l2_requestbuffers));

    reqbufs.count = 1;
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbufs.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fd, VIDIOC_REQBUFS, &reqbufs);
    if (ret) {
        printf("ERROR:(fd, VIDIOC_REQBUFS, &reqbufs) %d\n", ret);
        return 1;
    }

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(struct v4l2_buffer));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
    if (ret) {
        printf("ERROR:(fd, VIDIOC_QUERYBUF, &buf) %d\n", ret);
        return 1;
    }

    void *buffer_start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (buffer_start == MAP_FAILED) {
        printf("ERROR: mmap failed\n");
        return 1;
    }

    ret = ioctl(fd, VIDIOC_QBUF, &buf);
    if (ret) {
        printf("ERROR:(fd, VIDIOC_QBUF, &buf) %d\n", ret);
        return 1;
    }

    ret = ioctl(fd, VIDIOC_STREAMON, &buf.type);
    if (ret) {
        printf("ERROR:(fd, VIDIOC_STREAMON, &buf.type) %d\n", ret);
        return 1;
    }

    ret = ioctl(fd, VIDIOC_DQBUF, &buf);
    if (ret) {
        printf("ERROR:(fd, VIDIOC_DQBUF, &buf) %d\n", ret);
        return 1;
    }
    printf("Captured frame: %u bytes\n", buf.bytesused);

    FILE *out = fopen("frame.raw", "wb");
    if (!out) {
        printf("ERROR: fopen frame.raw failed\n");
        return 1;
    }
    fwrite(buffer_start, 1, buf.bytesused, out);
    fclose(out);

    ret = ioctl(fd, VIDIOC_STREAMOFF, &buf.type);
    if (ret) {
        printf("ERROR:(fd, VIDIOC_STREAMOFF, &buf.type) %d\n", ret);
        return 1;
    }

    munmap(buffer_start, buf.length);
    close(fd);

    printf("Done. Wrote frame.raw (%u bytes)\n", buf.bytesused);

    return 0;
}
