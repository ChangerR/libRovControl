#include "RovControl.h"


RovControl::RovControl() {
    _power = 0.1f;
    _running = false;
    _client = NULL;
    _updateTime = 50;
    _lastUpdateTime = 0;
}

RovControl::~RovControl() {
    close();
}

void RovControl::onClose(SIOClient* client) {
    _running = false;
}

void RovControl::onError(SIOClient* client, const std::string& data) {
    //_running = false;
    CCLOG("\nconnect occur error:%s\n",data.c_str());
}

void RovControl::setThrottle(float t) {
    _pos.throttle = t * _power;
}

void RovControl::setYaw(float y) {
    _pos.yaw = y * _power;
}

void RovControl::setLift(float l) {
    _pos.lift = l * _power;
}

void RovControl::setPower(float p) {
    _power = p;
}

bool RovControl::open(const char* url) {
    _url = url;
    _running = true;

    _client = SocketIO::connect(url,*this);

    return _client == NULL ? false : true;
}

void RovControl::stopAll() {
    _pos.throttle = 0.f;
    _pos.yaw = 0.f;
    _pos.lift = 0.f;
}

void RovControl::close() {
    _running = false;
    if(_client != NULL) {
        _client->release();
        _client = NULL;
    }
}

bool RovControl::processMessage() {

    long nowt = getTimeInMs();

    if(_lastUpdateTime + _updateTime < nowt) {
        if(!_pos.equals(_lastPos)&&_client&&_running) {
            _client->emit("control_update",_pos.toJSONString());
            _lastPos.assign(_pos);
        }
        _lastUpdateTime = nowt;
    }
    SocketIO::getInstance()->dispatchMessage();
    return _running;
}

void RovControl::setUpdateTime(int t) {
    _updateTime = t;
}

int RovControl::getUpdateTime() {
    return _updateTime;
}
