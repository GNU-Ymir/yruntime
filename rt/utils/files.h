#ifndef FILES_H_
#define FILES_H_
#include <sys/select.h>


void _yrt_fd_set (int fd, fd_set * set);
void _yrt_fd_zero (fd_set * set);
int _yrt_fd_isset (int fd, fd_set * set);


#endif // FILES_H_
