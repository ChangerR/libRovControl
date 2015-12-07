#ifndef __LIBSOCKETIO_ROVCONTROL__
#define __LIBSOCKETIO_ROVCONTROL__
#include "SocketIO.h"
#include <string>
#ifdef _WIN32
inline long getTimeInMs() {
    return GetTickCount();
}
#elif defined(__linux__)
#include <sys/time.h>
#include <time.h>
inline long getTimeInMs() {
    struct timeval t_start;
    gettimeofday(&t_start, NULL);

    return ((long)t_start.tv_sec)*1000+(long)(t_start.tv_usec/1000);
}
#endif

typedef float f32;

const f32 ROUNDING_ERROR_f32 = 0.000001f;

inline bool equals(const f32 a, const f32 b, const f32 tolerance = ROUNDING_ERROR_f32)
{
	return (a + tolerance >= b) && (a - tolerance <= b);
}

inline std::string& operator + (std::string& s,float f) {
    char tmp[32] = { 0 };
	sprintf(tmp, "%.2f", f);
    s += tmp;

    return s;
}

const float	FORWARD_VALUE = -1.0f;
const float BACK_VALUE = 1.0f;
const float LEFT_VALUE = -1.0f;
const float RIGHT_VALUE = 1.0f;
const float UP_VALUE = 1.0f;
const float DOWN_VALUE = -1.0f;

class RovControl
    :public SocketIO::SIODelegate {
public:

    class Position {
	public:
		Position():throttle(0.f),yaw(0.f),lift(0.f){}

		bool equals(const Position& p) const {
			bool ret = (::equals(p.lift,lift)&&::equals(p.throttle,throttle)&&::equals(p.yaw,yaw));
			//printf("A[throttle:%.2f,yaw:%.2f,lift:%.2f] B[throttle:%.2f,yaw:%.2f,lift:%.2f] ret=%s\n",throttle,yaw,lift,p.throttle,p.yaw,p.lift,ret ? "TRUE":"FALSE");
			return ret;
		}

		void assign(const Position& p) {
			this->lift = p.lift;
			this->throttle = p.throttle;
			this->yaw = p.yaw;
		}

		std::string toJSONString() const{
			std::string tmp = "{\"throttle\":\"";

            tmp = tmp + throttle;
			tmp.append("\",\"yaw\":\"");
            tmp = tmp + yaw;
			tmp.append("\",\"lift\":\"");
            tmp = tmp + lift;
			tmp.append("\"}");

			return tmp;
		}

	public:
		float throttle;
		float yaw;
		float lift;
	};
public:
    RovControl();
    virtual ~RovControl();

    virtual void onClose(SIOClient* client);

	virtual void onError(SIOClient* client, const std::string& data);

	void setThrottle(float t);

	void setYaw(float y);

	void setLift(float l);

	void stopAll();

	const Position& getPosition() const {
		return _pos;
	}

	void setPower(float p);

	float getPower() const{
		return _power;
	}

	bool open(const char* url);

	void close();

	bool processMessage();

	void setUpdateTime(int t);

	int getUpdateTime();

private:
	float _power;
	Position _pos;
	Position _lastPos;
	bool _running;
	std::string _url;
	SIOClient* _client;
	int _updateTime;
	long _lastUpdateTime;
};
#endif
