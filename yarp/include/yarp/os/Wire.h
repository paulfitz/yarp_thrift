// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#ifndef _YARP2_WIRE_
#define _YARP2_WIRE_

#include <yarp/os/api.h>
#include <yarp/os/Portable.h>
#include <yarp/os/Contactable.h>
#include <yarp/os/Port.h>
#include <yarp/os/Bottle.h>

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <set>

#include <stdio.h>

namespace yarp {
    namespace os {
        class NullConnectionReader;
        class NullConnectionWriter;
        class Wire;
        class WireState;
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
    virtual bool pushInt(int x) { return false; }
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

class yarp::os::WireState {
public:
    int len;
    int code;
    bool need_ok;
    WireState *parent;

    WireState() {
        len = -1;
        code = -1;
        need_ok = false;
        parent = 0 /*NULL*/;
    }

    bool isValid() const { 
        return len>=0; 
    }
};


class yarp::os::WireReader {
public:
    NullConnectionWriter null_writer;
    ConnectionReader& reader;
    WireState baseState;
    WireState *state;

    WireReader(ConnectionReader& reader) : reader(reader) {
        reader.convertTextMode();
        state = &baseState;
    }

    ~WireReader() {
        if (state->need_ok) {
            int32_t dummy;
            readVocab(dummy);
            state->need_ok = false;
        }
    }

    bool read(PortReader& obj) {
        state->len--;
        return obj.read(reader);
    }

    bool readI32(int32_t& x) {
        if (state->code<0) {
            int tag = reader.expectInt();
            if (tag!=BOTTLE_TAG_INT) return false;
        }
        int v = reader.expectInt();
        x = (int32_t) v;
        state->len--;
        return !reader.isError();
    }

    bool readBool(bool& x) {
        if (state->code<0) {
            int tag = reader.expectInt();
            if (tag!=BOTTLE_TAG_INT&&tag!=BOTTLE_TAG_VOCAB) return false;
        }
        int v = reader.expectInt();
        x = (v!=0) && (v!=VOCAB4('f','a','i','l'));
        state->len--;
        return !reader.isError();
    }

    bool readVocab(int32_t& x) {
        if (state->code<0) {
            int tag = reader.expectInt();
            if (tag!=BOTTLE_TAG_VOCAB) return false;
        }
        int v = reader.expectInt();
        x = (int32_t) v;
        state->len--;
        return !reader.isError();
    }

    bool readDouble(double& x) {
        if (state->code<0) {
            int tag = reader.expectInt();
            if (tag!=BOTTLE_TAG_DOUBLE) return false;
        }
        x = reader.expectDouble();
        state->len--;
        return !reader.isError();
    }

    bool readString(std::string& str, bool *is_vocab = 0 /*NULL*/) {
        int tag = state->code;
        if (state->code<0) {
            tag = reader.expectInt();
            if (tag!=BOTTLE_TAG_STRING&&tag!=BOTTLE_TAG_VOCAB) return false;
        }
        state->len--;
        if (tag==BOTTLE_TAG_VOCAB) {
            if (is_vocab) *is_vocab = true;
            NetInt32 v = reader.expectInt();
            if (reader.isError()) return false;
            str = Vocab::decode(v);
            return true;
        }
        if (is_vocab) *is_vocab = false;
        int len = reader.expectInt();
        if (reader.isError()) return false;
        if (len<1) return false;
        str.resize(len);
        reader.expectBlock((const char *)str.c_str(),len);
        str.resize(len-1);
        //printf("Read [%s]\n", str.c_str());
        return !reader.isError();
    }

    bool readListHeader() {
        int x1 = 0, x2 = 0;
        x1 = reader.expectInt();
        x2 = reader.expectInt();
        if (!(x1&BOTTLE_TAG_LIST)) return false;
        int code = x1&(~BOTTLE_TAG_LIST);
        state->len = x2;
        if (code!=0) state->code = code;
        return !reader.isError();
    }

    bool readListHeader(int len) {
        if (!readListHeader()) return false;
        return len == state->len;
    }

    bool readListReturn() {
        if (!readListHeader()) return false;
        if (state->len == 1) return true;
        if (state->len != 4) return false;
        // possibly old-style return: [is] foo val [ok]
        int32_t v = 0;
        if (!readVocab(v)) return false;
        if (v!=VOCAB2('i','s')) return false;
        std::string dummy;
        if (!readString(dummy)) return false; // string OR vocab
        // now we are ready to consume real result
        state->need_ok = true;
        return true;
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
        std::string str;
        bool is_vocab;
        if (!readString(str,&is_vocab)) return "";
        if (!is_vocab) return str.c_str();
        while (is_vocab&&state->len>0) {
            int x = reader.expectInt();
            reader.pushInt(x);
            is_vocab = (x==BOTTLE_TAG_VOCAB);
            if (is_vocab) {
                std::string str2;
                if (!readString(str2,&is_vocab)) return "";
                str += "_";
                str += str2;
            }
        }
        return str.c_str();
    }

    void readListBegin(WireState& nstate, uint32_t& len) {
        nstate.parent = state;
        state = &nstate;
        len = 0;
        readListHeader();
        len = (uint32_t)state->len;
    }

    void readListEnd() {
        state = state->parent;
    }
};

class yarp::os::WireWriter {
public:
    ConnectionWriter& writer;

    WireWriter(ConnectionWriter& writer) : writer(writer) {
        writer.convertTextMode();
    }

    WireWriter(WireReader& reader) : writer(reader.getWriter()) {
        writer.convertTextMode();
    }

    bool write(PortWriter& obj) {
        return obj.write(writer);
    }

    bool writeI32(int32_t x) {
        writer.appendInt(BOTTLE_TAG_INT);
        writer.appendInt((int)x);
        return !writer.isError();
    }

    bool writeBool(bool x) {
        writer.appendInt(BOTTLE_TAG_VOCAB);
        writer.appendInt(x?VOCAB2('o','k'):VOCAB4('f','a','i','l'));
        return !writer.isError();
    }

    bool writeDouble(double x) {
        writer.appendInt(BOTTLE_TAG_DOUBLE);
        writer.appendDouble(x);
        return !writer.isError();
    }

    bool isValid() {
        return writer.isValid();
    }

    bool isError() {
        return writer.isError();
    }

    bool writeTag(const char *tag, int split, int len) {
        if (!split) {
            return writeString(tag);
        }
        ConstString bit = "";
        char ch = 'x';
        while (ch!='\0') {
            ch = *tag;
            tag++;
            if (ch=='\0'||ch=='_') {
                writer.appendInt(BOTTLE_TAG_VOCAB);
                writer.appendInt(Vocab::encode(bit));
                bit = "";
            } else {
                bit += ch;
            }
        }
        return true;
    }

    bool writeString(const std::string& tag) {
        //printf("Writing [%s]\n", tag.c_str());
        writer.appendInt(BOTTLE_TAG_STRING);
        writer.appendInt((int)tag.length()+1);
        writer.appendString(tag.c_str(),'\0');
        return !writer.isError();
    }

    bool writeListHeader(int len) {
        writer.appendInt(BOTTLE_TAG_LIST);
        writer.appendInt(len);
        return !writer.isError();
    }


    bool writeListBegin(int tag, uint32_t len) {
        // this should be optimized for double/int/etc
        writer.appendInt(BOTTLE_TAG_LIST);
        writer.appendInt((int)len);
        return !writer.isError();
    }

    bool writeListEnd() {
        return true;
    }
};


#endif

