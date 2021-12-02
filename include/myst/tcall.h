// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _MYST_TCALL_H
#define _MYST_TCALL_H

#include <poll.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <myst/blockdevice.h>
#include <myst/defs.h>
#include <myst/fssig.h>

typedef enum myst_tcall_number
{
    MYST_TCALL_RANDOM = 2048,
    MYST_TCALL_VSNPRINTF,
    MYST_TCALL_WRITE_CONSOLE,
    MYST_TCALL_GEN_CREDS,
    MYST_TCALL_FREE_CREDS,
    MYST_TCALL_VERIFY_CERT,
    MYST_TCALL_GEN_CREDS_EX,
    MYST_TCALL_CLOCK_GETTIME,
    MYST_TCALL_CLOCK_SETTIME,
    MYST_TCALL_ISATTY,
    MYST_TCALL_ADD_SYMBOL_FILE,
    MYST_TCALL_LOAD_SYMBOLS,
    MYST_TCALL_UNLOAD_SYMBOLS,
    MYST_TCALL_CREATE_THREAD,
    MYST_TCALL_WAIT,
    MYST_TCALL_WAKE,
    MYST_TCALL_WAKE_WAIT,
    MYST_TCALL_SET_RUN_THREAD_FUNCTION,
    MYST_TCALL_TARGET_STAT,
    MYST_TCALL_SET_TSD,
    MYST_TCALL_GET_TSD,
    MYST_TCALL_GET_ERRNO_LOCATION,
    MYST_TCALL_READ_CONSOLE,
    MYST_TCALL_POLL_WAKE,
    MYST_TCALL_OPEN_BLOCK_DEVICE,
    MYST_TCALL_CLOSE_BLOCK_DEVICE,
    MYST_TCALL_READ_BLOCK_DEVICE,
    MYST_TCALL_WRITE_BLOCK_DEVICE,
    MYST_TCALL_LUKS_ENCRYPT,
    MYST_TCALL_LUKS_DECRYPT,
    MYST_TCALL_SHA256_START,
    MYST_TCALL_SHA256_UPDATE,
    MYST_TCALL_SHA256_FINISH,
    MYST_TCALL_VERIFY_SIGNATURE,
    MYST_TCALL_LOAD_FSSIG,
    MYST_TCALL_CLOCK_GETRES,
    MYST_TCALL_GCOV,
    MYST_TCALL_INTERRUPT_THREAD,
    MYST_TCALL_CONNECT_BLOCK,
    MYST_TCALL_ACCEPT4_BLOCK,
    MYST_TCALL_READ_BLOCK,
    MYST_TCALL_WRITE_BLOCK,
    MYST_TCALL_RECVFROM_BLOCK,
    MYST_TCALL_SENDTO_BLOCK,
    MYST_TCALL_RECVMSG_BLOCK,
    MYST_TCALL_SENDMSG_BLOCK,
    /* Open Enclave internal APIs */
    MYST_TCALL_TD_SET_EXCEPTION_HANDLER_STACK,
    MYST_TCALL_TD_REGISTER_EXCEPTION_HANDLER_STACK,
    MYST_TCALL_MASK_HOST_SIGNAL,
    MYST_TCALL_UNMASK_HOST_SIGNAL,
    MYST_TCALL_REGISTER_HOST_SIGNAL,
    MYST_TCALL_UNREGISTER_HOST_SIGNAL,
    MYST_TCALL_HOST_SIGNAL_REGISTERED,
    MYST_TCALL_IS_HANDLING_HOST_SIGNAL,
} myst_tcall_number_t;

long myst_tcall(long n, long params[6]);

typedef long (*myst_tcall_t)(long n, long params[6]);

long myst_tcall_random(void* data, size_t size);

long myst_tcall_thread_self(void);

long myst_tcall_vsnprintf(
    char* str,
    size_t size,
    const char* format,
    va_list ap);

long myst_tcall_read_console(int fd, void* buf, size_t count);

long myst_tcall_write_console(int fd, const void* buf, size_t count);

long myst_tcall_create_thread(uint64_t cookie);

/* returns zero or -errno */
long myst_tcall_wait(uint64_t event, const struct timespec* timeout);

/* returns the number of waiters that were woken up or -errno */
long myst_tcall_wake(uint64_t event);

/* returns zero or -errno */
long myst_tcall_wake_wait(
    uint64_t waiter_event,
    uint64_t self_event,
    const struct timespec* timeout);

long myst_tcall_add_symbol_file(
    const void* file_data,
    size_t file_size,
    const void* text_data,
    size_t text_size,
    const char* enclave_rootfs_path);

long myst_tcall_load_symbols(void);

long myst_tcall_unload_symbols(void);

typedef long (
    *myst_run_thread_t)(uint64_t cookie, uint64_t event, pid_t target_tid);

long myst_tcall_set_run_thread_function(myst_run_thread_t function);

/* for getting statistical information from the target */
typedef struct myst_target_stat
{
    uint64_t heap_size; /* 0 indicates unbounded */
} myst_target_stat_t;

long myst_tcall_target_stat(myst_target_stat_t* buf);

/* set the thread-specific-data slot in the target (only one slot) */
long myst_tcall_set_tsd(uint64_t value);

/* get the thread-specific-data slot in the target (only one slot) */
long myst_tcall_get_tsd(uint64_t* value);

/* get the address of the thread-specific errno variable */
long myst_tcall_get_errno_location(int** ptr);

/* break out of poll() */
long myst_tcall_poll_wake(void);

long myst_tcall_poll(struct pollfd* fds, nfds_t nfds, int timeout);

int myst_tcall_open_block_device(const char* path, bool read_only);

int myst_tcall_close_block_device(int blkdev);

int myst_tcall_write_block_device(
    int blkdev,
    uint64_t blkno,
    const struct myst_block* blocks,
    size_t num_blocks);

ssize_t myst_tcall_read_block_device(
    int blkdev,
    uint64_t blkno,
    struct myst_block* blocks,
    size_t num_blocks);

int myst_tcall_verify_signature(
    const char* pem_public_key,
    const uint8_t* hash,
    size_t hash_size,
    const uint8_t* signer,
    size_t signer_size,
    const uint8_t* signature,
    size_t signature_size);

int myst_tcall_load_fssig(const char* path, myst_fssig_t* fssig);

int myst_tcall_mprotect(void* addr, size_t len, int prot);

long myst_gcov(const char* func, long params[6]);

long myst_tcall_close(int fd);

long myst_tcall_fcntl(int fd, int cmd, long arg);

long myst_tcall_fstat(int fd, struct stat* statbuf);

long myst_tcall_dup(int oldfd);

long myst_tcall_read(int fd, void* buf, size_t count);

long myst_tcall_write(int fd, const void* buf, size_t count);

long myst_tcall_poll(struct pollfd* fds, nfds_t nfds, int timeout);

long myst_tcall_pipe2(int pipefd[2], int flags);

long myst_tcall_nanosleep(const struct timespec* req, struct timespec* rem);

long myst_tcall_epoll_wait(
    int epfd,
    struct epoll_event* events,
    size_t maxevents,
    int timeout);

long myst_tcall_interrupt_thread(pid_t tid);

int myst_tcall_accept4(
    int sockfd,
    struct sockaddr* addr,
    socklen_t* addrlen,
    int flags);

int myst_tcall_connect(
    int sockfd,
    const struct sockaddr* addr,
    socklen_t addrlen);

ssize_t myst_tcall_sendto(
    int sockfd,
    const void* buf,
    size_t len,
    int flags,
    const struct sockaddr* dest_addr,
    socklen_t addrlen);

ssize_t myst_tcall_recvfrom(
    int sockfd,
    void* buf,
    size_t len,
    int flags,
    struct sockaddr* src_addr,
    socklen_t* addrlen);

ssize_t myst_tcall_sendmsg(int sockfd, const struct msghdr* msg, int flags);

ssize_t myst_tcall_recvmsg(int sockfd, struct msghdr* msg, int flags);

long myst_tcall_read_block(int fd, void* buf, size_t count);

long myst_tcall_write_block(int fd, const void* buf, size_t count);

int myst_tcall_accept4_block(
    int sockfd,
    struct sockaddr* addr,
    socklen_t* addrlen,
    int flags);

int myst_tcall_connect_block(
    int sockfd,
    const struct sockaddr* addr,
    socklen_t addrlen);

ssize_t myst_tcall_sendto_block(
    int sockfd,
    const void* buf,
    size_t len,
    int flags,
    const struct sockaddr* dest_addr,
    socklen_t addrlen);

ssize_t myst_tcall_recvfrom_block(
    int sockfd,
    void* buf,
    size_t len,
    int flags,
    struct sockaddr* src_addr,
    socklen_t* addrlen);

ssize_t myst_tcall_sendmsg_block(
    int sockfd,
    const struct msghdr* msg,
    int flags);

ssize_t myst_tcall_recvmsg_block(int sockfd, struct msghdr* msg, int flags);

long myst_tcall_td_set_exception_handler_stack(
    void* td,
    void* stack,
    size_t size);

long myst_tcall_td_register_exception_handler_stack(void* td, uint64_t type);

int myst_tcall_tgkill(pid_t tgid, pid_t tid, int sig);

long myst_tcall_mask_host_signal(void* td);

long myst_tcall_unmask_host_signal(void* td);

long myst_tcall_register_host_signal(void* td, int signo);

long myst_tcall_unregister_host_signal(void* td, int signo);

long myst_tcall_host_signal_registered(void* td, int signo);

long myst_tcall_is_handling_host_signal(void* td);

#endif /* _MYST_TCALL_H */
