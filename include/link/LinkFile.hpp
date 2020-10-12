/*! \file */ // Copyright 2011-2020 Tyler Gilbert and Stratify Labs, Inc; see
             // LICENSE.md for rights.

#ifndef SAPI_FS_FILE_HPP_
#define SAPI_FS_FILE_HPP_

#include <sos/link.h>

#include "api/api.hpp"
#include "fs/File.hpp"

#include "LinkPath.hpp"

namespace linkns {

class LinkFile : public fs::FileAccess<LinkFile> {
public:
  enum class IsOverwrite { no, yes };

  enum class Whence {
    set /*! Set the location of the file descriptor */ = LINK_SEEK_SET,
    current /*! Set the location relative to the current location */
    = LINK_SEEK_CUR,
    end /*! Set the location relative to the end of the file or device */
    = LINK_SEEK_END
  };

  LinkFile(link_transport_mdriver_t *driver = nullptr);
  LinkFile(var::StringView name, OpenMode flags = OpenMode::read_write(),
           link_transport_mdriver_t *driver = nullptr);

  LinkFile(const LinkFile &file) = delete;
  LinkFile &operator=(const LinkFile &file) = delete;

  LinkFile(LinkFile &&file) = default;
  LinkFile &operator=(LinkFile &&file) = default;

  static File create(var::StringView path, IsOverwrite is_overwrite,
                     Permissions perms = Permissions(0666)
                         FSAPI_LINK_DECLARE_DRIVER_NULLPTR_LAST);

protected:
  API_AF(File, link_transport_mdriver_t *, driver, nullptr);

  virtual int interface_open(const char *path, int flags, int mode) const;
  virtual int interface_lseek(int fd, int offset, int whence) const;
  virtual int interface_read(int fd, void *buf, int nbyte) const;
  virtual int interface_write(int fd, const void *buf, int nbyte) const;
  virtual int interface_ioctl(int fd, int request, void *argument) const;
  virtual int interface_close(int fd) const;
  virtual int interface_fsync(int fd) const;

private:
};

} // namespace linkns

#endif /* SAPI_FS_FILE_HPP_ */
