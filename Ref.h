#ifndef __LIBSOCKETIO_REF_H
#define __LIBSOCKETIO_REF_H

class Ref {
public:
    Ref():_count(1){
    }
    virtual ~Ref(){}

    int retain() {
        ++_count;
        return _count;
    }

    void release() {
        --_count;

        if(_count <= 0)
            delete this;
    }

protected:
    volatile int _count;
};
#endif
