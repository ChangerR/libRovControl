CPP = g++
CPPFLAGS = -I libwebsockets/linux-x64/include -I . -I rapidjson/include -c -g
LINK = g++
OBJS = websocket.o HttpClient.o HttpCookie.o SocketIO.o RovControl.o
LKFLAGS = -L libwebsockets/linux-x64/lib64 -lwebsockets -lcurl -lpthread

TARGET = test1 test2 test3 test4

all:$(TARGET)
	@echo "Make all target"

test1: $(OBJS) test/test1.o
	$(LINK) -o $@ $(OBJS) test/test1.o $(LKFLAGS)

test2:test/test2.o $(OBJS)
	$(LINK) -o $@ test/test2.o $(OBJS) $(LKFLAGS)

test3:test/test3.o $(OBJS)
	$(LINK) -o $@ test/test3.o $(OBJS) $(LKFLAGS)

test4:test/test4.o $(OBJS)
	$(LINK) -o $@ test/test4.o $(OBJS) $(LKFLAGS)

%.o:%.cpp
	g++ $(CPPFLAGS) -o $@ $<

clean:
	-rm *.o
	-rm $(TARGET)
	cd test&&rm *.o
