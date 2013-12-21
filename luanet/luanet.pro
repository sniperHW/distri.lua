TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += \
    ../kendynet/core/src/wpacket.c \
    ../kendynet/core/src/uthread.c \
    ../kendynet/core/src/timer.c \
    ../kendynet/core/src/thread.c \
    ../kendynet/core/src/SysTime.c \
    ../kendynet/core/src/spinlock.c \
    ../kendynet/core/src/sock_util.c \
    ../kendynet/core/src/Socket.c \
    ../kendynet/core/src/rpacket.c \
    ../kendynet/core/src/RBtree.c \
    ../kendynet/core/src/netservice.c \
    ../kendynet/core/src/msg_que.c \
    ../kendynet/core/src/KendyNet.c \
    ../kendynet/core/src/except.c \
    ../kendynet/core/src/epoll.c \
    ../kendynet/core/src/Engine.c \
    ../kendynet/core/src/Connection.c \
    ../kendynet/core/src/buffer.c \
    ../kendynet/test/testmq.c

HEADERS += \
    ../kendynet/core/wpacket.h \
    ../kendynet/core/uthread.h \
    ../kendynet/core/timer.h \
    ../kendynet/core/thread.h \
    ../kendynet/core/SysTime.h \
    ../kendynet/core/sync.h \
    ../kendynet/core/spinlock.h \
    ../kendynet/core/sock_util.h \
    ../kendynet/core/rpacket.h \
    ../kendynet/core/refbase.h \
    ../kendynet/core/RBtree.h \
    ../kendynet/core/packet.h \
    ../kendynet/core/netservice.h \
    ../kendynet/core/msg_que.h \
    ../kendynet/core/link_list.h \
    ../kendynet/core/KendyNet.h \
    ../kendynet/core/exception.h \
    ../kendynet/core/except.h \
    ../kendynet/core/double_link.h \
    ../kendynet/core/Connection.h \
    ../kendynet/core/common_define.h \
    ../kendynet/core/common.h \
    ../kendynet/core/buffer.h \
    ../kendynet/core/atomic_st.h \
    ../kendynet/core/atomic.h \
    ../kendynet/core/allocator.h \
    ../kendynet/core/src/uthread_ucontext.h \
    ../kendynet/core/src/uthread_64.h \
    ../kendynet/core/src/uthread_32.h \
    ../kendynet/core/src/Socket.h \
    ../kendynet/core/src/epoll.h \
    ../kendynet/core/src/Engine.h

