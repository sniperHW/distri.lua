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
    ../kendynet/test/testmq.c \
    ../kendynet/test/benchserver.c \
    ../kendynet/test/testtimer.c \
    ../kendynet/test/testgetsystick.c \
    ../kendynet/test/tcpserver.c \
    ../kendynet/test/tcpclient.c \
    ../kendynet/test/netclient.c \
    ../kendynet/test/netboradcast.c \
    ../kendynet/test/benchclient.c \
    ../kendynet/core/src/datasocket.c \
    ../kendynet/core/src/coronet.c \
    ../kendynet/core/src/refbase.c \
    ../luanet.c \
    ../kendynet/core/src/systime.c \
    ../kendynet/core/src/rbtree.c \
    ../kendynet/core/src/poller.c \
    ../kendynet/core/src/msgque.c \
    ../kendynet/core/src/kendynet.c \
    ../kendynet/core/src/connection.c \
    ../kendynet/core/src/asynsock.c \
    ../kendynet/core/src/asynnet.c \
    ../kendynet/core/src/socket.c \
    ../kendynet/test/test_atomic_st.c \
    ../kendynet/core/src/atomic_st.c \
    ../kendynet/core/src/lprocess.c \
    ../kendynet/core/src/minheap.c

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
    ../kendynet/core/src/Engine.h \
    ../kendynet/test/testcommon.h \
    ../kendynet/core/datasocket.h \
    ../kendynet/core/coronet.h \
    ../kendynet/core/systime.h \
    ../kendynet/core/rbtree.h \
    ../kendynet/core/msgque.h \
    ../kendynet/core/kendynet.h \
    ../kendynet/core/connection.h \
    ../kendynet/core/asynnet.h \
    ../kendynet/core/src/poller.h \
    ../kendynet/core/src/asynsock.h \
    ../kendynet/core/src/socket.h \
    ../kendynet/core/llist.h \
    ../kendynet/core/dlist.h \
    ../kendynet/core/lprocess.h \
    ../kendynet/core/minheap.h

