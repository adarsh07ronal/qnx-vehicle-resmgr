/* qnx_stubs/sys/iofunc.h
 * Minimal QNX iofunc stubs for Linux host compilation.
 * Lets the code compile on CI/Linux so logic and structure can be verified
 * without a QNX SDP license. Real QNX build uses qcc + genuine headers.
 */
#ifndef _QNX_STUB_IOFUNC_H
#define _QNX_STUB_IOFUNC_H

#include <stdint.h>
#include <sys/stat.h>

/* ── Types ── */
typedef struct { int id; } resmgr_context_t;
typedef struct { int dummy; } resmgr_attr_t;
typedef struct { int dummy; } dispatch_t;
typedef struct { void *ptr; } dispatch_context_t;

typedef struct _iofunc_attr {
    mode_t  mode;
    int     flags;
} iofunc_attr_t;

typedef struct _iofunc_ocb {
    iofunc_attr_t *attr;
    off_t          offset;
} iofunc_ocb_t;

#define RESMGR_OCB_T iofunc_ocb_t

typedef struct { int nfuncs; } iofunc_funcs_t;
#define _IOFUNC_NFUNCS 16

typedef struct {
    int  (*read) (resmgr_context_t*, void*, iofunc_ocb_t*);
    int  (*write)(resmgr_context_t*, void*, iofunc_ocb_t*);
} resmgr_io_funcs_t;

typedef struct { uint32_t nbytes; } io_read_t;
typedef struct { struct { uint32_t nbytes; } i; } io_write_t;

typedef struct { void *base; size_t len; } iov_t;

/* ── Macros ── */
#define S_IFCHR         0020000
#define _FTYPE_ANY      0
#define _RESMGR_NPARTS(n)   (n)
#define _RESMGR_NFUNCS      16
#define _RESMGR_CONNECT_NFUNCS 8
#define _RESMGR_IO_NFUNCS   16
#define MSG_NOSIGNAL        0
#define SETIOV(iov, b, l)   do { (iov)->base=(b); (iov)->len=(l); } while(0)
#define _IO_SET_WRITE_NBYTES(c,n) ((void)(n))

/* ── Stub functions ── */
static inline void iofunc_attr_init(iofunc_attr_t *a, mode_t m, void *u, void *g)
    { (void)u; (void)g; a->mode = m; a->flags = 0; }

static inline void iofunc_func_init(int cn, void *cf, int in, resmgr_io_funcs_t *io)
    { (void)cn; (void)cf; (void)in; (void)io; }

static inline dispatch_t *dispatch_create(void)
    { static dispatch_t d; return &d; }

static inline int resmgr_attach(dispatch_t *d, resmgr_attr_t *a, const char *path,
    int type, int flags, resmgr_io_funcs_t *cf, iofunc_funcs_t *io, iofunc_attr_t *attr)
    { (void)d;(void)a;(void)path;(void)type;(void)flags;(void)cf;(void)io;(void)attr; return 0; }

static inline dispatch_context_t *dispatch_context_alloc(dispatch_t *d)
    { (void)d; static dispatch_context_t c; return &c; }

static inline dispatch_context_t *dispatch_block(dispatch_context_t *c)
    { (void)c; return NULL; } /* returns NULL → exits loop in host build */

static inline void dispatch_handler(dispatch_context_t *c) { (void)c; }

#endif /* _QNX_STUB_IOFUNC_H */
