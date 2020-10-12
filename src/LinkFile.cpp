

#include "link/LinkFile.hpp"

int LinkFile::interface_open(const char *path, int flags, int mode) const {
  return link_open(driver(), path, flags, mode);
}

int LinkFile::interface_lseek(int fd, int offset, int whence) const {
  return link_lseek(driver(), fd, offset, whence);
}

int LinkFile::interface_read(int fd, void *buf, int nbyte) const {
  return link_read(driver(), fd, buf, nbyte);
}

int LinkFile::interface_write(int fd, const void *buf, int nbyte) const {
  return link_write(driver(), fd, buf, nbyte);
}

int LinkFile::interface_ioctl(int fd, int request, void *argument) const {
  return link_ioctl(driver(), fd, request, argument);
}

int LinkFile::interface_close(int fd) const { return link_close(driver(), fd); }

int LinkFile::interface_fsync(int fd) const { return -1; }
