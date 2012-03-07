// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#ifndef _YARP2_WIRE_
#define _YARP2_WIRE_

#include <yarp/os/api.h>
#include <yarp/os/Portable.h>
#include <yarp/os/Contactable.h>
#include <yarp/os/Port.h>

#include <stdint.h>
#include <string>

namespace yarp {
    namespace os {
        class NullConnectionReader;
        class NullConnectionWriter;
        class Wire;
        class WireReader;
        class WireWriter;
    }
}

class yarp::os::NullConnectionWriter : public ConnectionWriter {
public:
    virtual void appendBlock(const char *data, size_t len) {}
    virtual void appendInt(int data) {}
    virtual void appendDouble(double data) {}
    virtual void appendString(const char *str, int terminate = '\n') {}
    virtual void appendExternalBlock(const char *data, size_t len) {}
    virtual bool isTextMode() { return false; }
    virtual void declareSizes(int argc, int *argv) {}
    virtual void setReplyHandler(PortReader& reader) {}
    virtual void setReference(Portable *obj) {}
    virtual bool convertTextMode() { return false; }
    virtual bool isValid() { return false; }
    virtual bool isActive() { return true; }
    virtual bool isError() { return true; }
    virtual void requestDrop() { }
};


class yarp::os::NullConnectionReader : public ConnectionReader {
public:

    virtual bool expectBlock(const char *data, size_t len) { return false; }
    virtual ConstString expectText(int terminatingChar = '\n') { return ""; }
    virtual int expectInt() { return 0; }
    virtual double expectDouble() { return 0.0; }
    virtual bool isTextMode() { return false; }
    virtual bool convertTextMode() { return false; }
    virtual size_t getSize() { return 0; }
    virtual ConnectionWriter *getWriter() { return 0/*NULL*/; }
    virtual Bytes readEnvelope() { return Bytes(0,0); }
    virtual Portable *getReference() { return 0/*NULL*/; }
    virtual Contact getRemoteContact() { return Contact(); }
    virtual Contact getLocalContact() { return Contact(); }
    virtual bool isValid() { return false; }
    virtual bool isActive() { return false; }
    virtual bool isError() { return true; }
    virtual void requestDrop() {}
};


class yarp::os::WireReader {
public:
    NullConnectionWriter null_writer;
    ConnectionReader& reader;
    WireReader(ConnectionReader& reader) : reader(reader) {}

    bool read(PortReader& obj) {
        return obj.read(reader);
    }

    bool readI32(int32_t& x) {
        int v = reader.expectInt();
        x = (int32_t) v;
        return !reader.isError();
    }

    bool readString(std::string& str) {
        yarp::os::ConstString cstr = reader.expectText('\0');
        str = std::string(cstr.c_str(), cstr.length());
        return !reader.isError();
    }

    ConnectionWriter& getWriter() {
        ConnectionWriter *writer = reader.getWriter();
        if (writer) return *writer;
        return null_writer;
    }

    bool isValid() {
        return reader.isValid();
    }

    bool isError() {
        return reader.isError();
    }

    yarp::os::ConstString readTag() {
        return reader.expectText('\0');
    }
};

class yarp::os::WireWriter {
public:
    ConnectionWriter& writer;

    WireWriter(ConnectionWriter& writer) : writer(writer) {}
    WireWriter(WireReader& reader) : writer(reader.getWriter()) {}

    bool write(PortWriter& obj) {
        return obj.write(writer);
    }

    bool writeI32(int32_t x) {
        writer.appendInt((int)x);
        return !writer.isError();
    }

    bool isValid() {
        return writer.isValid();
    }

    bool isError() {
        return writer.isError();
    }

    bool writeTag(const char *tag) {
        writer.appendString(tag,'\0');
        return !writer.isError();
    }

    bool writeString(const std::string& tag) {
        writer.appendString(tag.c_str(),'\0');
        return !writer.isError();
    }
};


#endif

