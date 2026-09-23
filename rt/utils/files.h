#ifndef FILES_H_
#define FILES_H_
#include <sys/select.h>
#include <stdint.h>
#include <rt/memory/types.h>


void _yrt_fd_set (int fd, fd_set * set);
void _yrt_fd_zero (fd_set * set);
int _yrt_fd_isset (int fd, fd_set * set);


char _yrt_file_date (char * path, int64_t * sec, uint64_t * nsec);
char _yrt_exists (char * path, char followLink);
int _yrt_file_size (char * path, uint64_t * size);
char _yrt_file_mode (char * path, char followLink, uint32_t * mode);
char _yrt_create_file (char * path, int32_t mode);
char _yrt_is_file (char * path, char followLink);
char _yrt_is_link (char * path);
char _yrt_is_dir (char * path, char followLink);

char _yrt_is_executable (char * path);
char _yrt_is_writable (char * path);
char _yrt_is_readable (char * path);

_yrt_slice_t _yrt_read_link (char * path);
_yrt_slice_t _yrt_real_path (char * path);

#endif // FILES_H_
