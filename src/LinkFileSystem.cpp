
#include <sys/stat.h>

#include "fs/Dir.hpp"
#include "var/Tokenizer.hpp"

#include "fs/FileSystem.hpp"
#include "local.h"

using namespace fs;

FileSystem::FileSystem(FSAPI_LINK_DECLARE_DRIVER) {
  LINK_SET_DRIVER((*this), link_driver);
}

FileSystem &FileSystem::remove(var::StringView path) {
  API_SYSTEM_CALL("", FSAPI_LINK_UNLINK(driver(), path.cstring()));
  return *this;
}

FileSystem &FileSystem::copy(const Copy &options) {
  File source(options.source_path(), OpenMode::read_only()
#if defined __link
                                         ,
              options.source_driver()
#endif
  );

  File dest(options.destination_path(), OpenMode::read_only()
#if defined __link
                                            ,
            options.destination_driver()
#endif
  );

  if (API_SYSTEM_CALL("", source.status().value()) < 0) {
    return *this;
  }

  if (API_SYSTEM_CALL("", dest.status().value()) < 0) {
    return *this;
  }

  return copy(source, dest, options);
}

FileSystem &FileSystem::copy(File &source, File &destination,
                             const Copy &options) {
  u32 mode = 0;

  FileInfo source_info = source.get_info();
  if (API_SYSTEM_CALL("", source.status().value())) {
    return *this;
  }

  mode = source_info.permissions().permissions();

  if (mode == 0) {
    mode = 0666;
  }

  if (API_SYSTEM_CALL("",
                      destination
                          .create(options.destination_path(),
                                  options.overwrite(), Permissions(mode & 0777))
                          .status()
                          .value()) < 0) {
    return *this;
  }

  API_SYSTEM_CALL("", destination
                          .write(source, File::Write().set_progress_callback(
                                             options.progress_callback()))
                          .status()
                          .value());

  return *this;
}

FileSystem &FileSystem::touch(var::StringView path) {
  char c;
  API_SYSTEM_CALL(
      "", File(path, OpenMode::read_write() FSAPI_LINK_MEMBER_DRIVER_LAST)
              .read(var::View(c))
              .seek(0)
              .write(var::View(c))
              .status()
              .value());
  return *this;
}

FileSystem &FileSystem::rename(const Rename &options) {
#if defined __link
  if (driver() == nullptr) {
    API_SYSTEM_CALL("", ::rename(options.source().cstring(),
                                 options.destination().cstring()));
    return *this;
  }
#endif
  API_SYSTEM_CALL("", FSAPI_LINK_RENAME(driver(), options.source().cstring(),
                                        options.destination().cstring()));
  return *this;
}

bool FileSystem::exists(var::StringView path) const {
  return File(path, OpenMode::read_only() FSAPI_LINK_MEMBER_DRIVER_LAST)
      .status()
      .is_success();
}

FileInfo FileSystem::get_info(var::StringView path) {
  API_RETURN_VALUE_IF_ERROR(FileInfo());

  struct FSAPI_LINK_STAT_STRUCT stat = {0};
  API_SYSTEM_CALL("get stat", FSAPI_LINK_STAT(driver(), path.cstring(), &stat));
  return FileInfo(stat);
}

FileInfo FileSystem::get_info(int fd) {
  API_RETURN_VALUE_IF_ERROR(FileInfo());
  struct FSAPI_LINK_STAT_STRUCT stat = {0};
  API_SYSTEM_CALL("get info fstat", FSAPI_LINK_FSTAT(driver(), fd, &stat));
  return FileInfo(stat);
}

FileSystem &FileSystem::copy_directory(const Copy &options) {

  var::Vector<var::String> source_contents =
      read_directory(options.source_path(), Recursive::no);

  for (auto entry : source_contents.vector()) {
    var::String entry_path = options.source_path() + "/" + entry;

    var::String destination_entry_path =
        options.destination_path() + "/" + entry;

    FileInfo info = FileSystem(
#if __link
                        options.source_driver()
#endif
                            )
                        .get_info(entry_path.cstring());

    if (info.is_directory()) {
      // copy recursively

      if (FileSystem(
#if __link
              options.destination_driver()
#endif
                  )
              .create_directory(destination_entry_path.cstring(),
                                Permissions::all_access(), Recursive::yes)
              .status()
              .is_error()) {
        // error here
        return *this;
      }

      copy_directory(Copy()
                         .set_source_path(entry_path.cstring())
                         .set_destination_path(destination_entry_path.cstring())
                         .set_progress_callback(options.progress_callback())
#if defined __link
                         .set_source_driver(options.source_driver())
                         .set_destination_driver(options.destination_driver())
#endif
      );

      // check result for errors

    } else if (info.is_file()) {
      copy(Copy()
               .set_source_path(entry_path.cstring())
               .set_destination_path(destination_entry_path.cstring())
               .set_overwrite(options.overwrite())
               .set_progress_callback(options.progress_callback())
#if defined __link
               .set_source_driver(options.source_driver())
               .set_destination_driver(options.destination_driver())
#endif
      );
      if (status().is_error()) {
        return *this;
      }
    }
  }
  return *this;
}

FileSystem &FileSystem::remove_directory(var::StringView path,
                                         Recursive recursive) {
  int ret = 0;
  if (recursive == Recursive::yes) {
    Dir d(path);
    if (d.status().is_success()) {
      var::String entry;
      while ((entry = d.read()).is_empty() == false) {
        var::String entry_path = path + "/" + entry;
        FileInfo info = get_info(entry_path.cstring());
        if (info.is_directory()) {
          if (entry != "." && entry != "..") {
            if (remove_directory(entry_path.cstring(), recursive)
                    .status()
                    .is_error()) {
              return *this;
            }
          }
        } else {
          if (remove(entry_path.cstring()).status().is_error()) {
            return *this;
          }
        }
      }
    }
  }

  if (status().is_success()) {
    // this will remove an empty directory or a file
#if defined __link
    if (driver()) {
      remove(path);
    } else {
      API_SYSTEM_CALL("", ::rmdir(path.cstring()));
    }
#else
    remove(path);
#endif
  }

  return *this;
}

var::StringList
FileSystem::read_directory(var::StringView path,
                           var::StringView (*filter)(var::StringView entry),
                           Recursive is_recursive) {
  return Dir(path).read_list(filter, is_recursive);
}

var::StringList FileSystem::read_directory(const fs::Dir &directory,
                                           Recursive is_recursive) {
  return read_directory(directory, nullptr, is_recursive);
}

u32 FileSystem::size(var::StringView name) { return get_info(name).size(); }

bool FileSystem::directory_exists(var::StringView path) {
  API_RETURN_VALUE_IF_ERROR(false);
  return Dir(path).status().is_success();
}

FileSystem &FileSystem::create_directory(var::StringView path,
                                         const Permissions &permissions) {
  int result;

#if defined __link
  if (driver()) {
    result = link_mkdir(driver(), path.cstring(), permissions.permissions());
  } else {
    // open a directory on the local system (not over link)
#if defined __win32
    result = mkdir(path.cstring());
#else
    result = mkdir(path.cstring(), permissions.permissions());
#endif
  }
#else
  result = mkdir(path.cstring(), permissions.permissions());
#endif
  API_SYSTEM_CALL("", result);
  return *this;
}

FileSystem &FileSystem::create_directory(var::StringView path,
                                         const Permissions &permissions,
                                         Recursive is_recursive) {
  if (is_recursive == Recursive::no) {
    return create_directory(path, permissions);
  }
  var::Tokenizer path_tokens =
      var::Tokenizer(path, var::Tokenizer::Construct().set_delimeters("/"));
  var::String base_path;

  if (path.find("/") == 0) {
    base_path += "/";
  }

  for (u32 i = 0; i < path_tokens.count(); i++) {
    if (path_tokens.at(i).is_empty() == false) {
      base_path += path_tokens.at(i);
      if (create_directory(base_path.cstring(), permissions)
              .status()
              .is_error()) {
        return *this;
      }
      base_path += "/";
    }
  }

  return *this;
}

#if !defined __link
int FileSystem::access(var::StringView path, const Access &access) {
  return ::access(path.cstring(), static_cast<int>(access.o_access()));
}
#endif
