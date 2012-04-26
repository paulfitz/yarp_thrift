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
        class WireLink;
        class WirePortable;
    }
}

class yarp::os::WirePortable : public Portable {
public:
    virtual bool read(yarp::os::WireReader& reader) = 0;
    virtual bool write(yarp::os::WireWriter& writer) = 0;
};

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
    bool flush_if_needed;
    bool get_mode;
    std::string get_string;
    bool get_is_vocab;
    bool support_get_mode;

    WireReader(ConnectionReader& reader) : reader(reader) {
        reader.convertTextMode();
        state = &baseState;
        size_t pending = reader.getSize();
        flush_if_needed = false;
        get_mode = false;
        support_get_mode = false;
    }

    ~WireReader() {
        if (state->need_ok) {
            int32_t dummy;
            readVocab(dummy);
            state->need_ok = false;
        }
        if (flush_if_needed) {
            clear();
        }
    }

    void allowGetMode() {
        support_get_mode = true;
    }

    bool clear() {
        size_t pending = reader.getSize();
        if (pending>0) {
            while (pending>0) {
                char buf[1000];
                size_t next = (pending<sizeof(buf))?pending:sizeof(buf);
                reader.expectBlock(&buf[0],next);
                pending -= next;
            }
            return true;
        }
        return false;
    }

    void fail() {
        clear();
        Bottle b("[fail]");
        b.write(getWriter());
    }


    bool read(WirePortable& obj) {
        return obj.read(*this);
    }

    //bool read(PortReader& obj) {
    //  state->len--;
    //  return obj.read(reader);
    //}

    bool readI32(int32_t& x) {
        size_t pending = reader.getSize();
        int tag = state->code;
        if (tag<0) {
            tag = reader.expectInt();
        }
        if (tag!=BOTTLE_TAG_INT) return false;
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
        int tag = state->code;
        if (tag<0) {
            tag = reader.expectInt();
        }
        if (tag!=BOTTLE_TAG_VOCAB) return false;
        int v = reader.expectInt();
        x = (int32_t) v;
        state->len--;
        return !reader.isError();
    }

    bool readDouble(double& x) {
        int tag = state->code;
        if (tag<0) {
            tag = reader.expectInt();
        }
        if (tag==BOTTLE_TAG_INT) {
            int v = reader.expectInt();
            x = v;
            state->len--;
            return !reader.isError();
        }
        if (tag!=BOTTLE_TAG_DOUBLE) return false;
        x = reader.expectDouble();
        state->len--;
        return !reader.isError();
    }

    bool readString(std::string& str, bool *is_vocab = 0 /*NULL*/) {
        if (state->len<=0) return false;
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
        return !reader.isError();
    }

    bool readListHeader() {
        int x1 = 0, x2 = 0;
        x1 = reader.expectInt();
        if (!(x1&BOTTLE_TAG_LIST)) return false;
        x2 = reader.expectInt();
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
        if (!support_get_mode) return true;
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
        flush_if_needed = false;
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
        flush_if_needed = true;
        std::string str;
        bool is_vocab;
        if (!readString(str,&is_vocab)) {
            fail();
            return "";
        }
        scanString(str,is_vocab);
        if (!is_vocab) return str.c_str();
        while (is_vocab&&state->len>0) {
            if (state->code>=0) {
                is_vocab = (state->code==BOTTLE_TAG_VOCAB);
            } else {
                int x = reader.expectInt();
                reader.pushInt(x);
                is_vocab = (x==BOTTLE_TAG_VOCAB);
            }
            if (is_vocab) {
                std::string str2;
                if (!readString(str2,&is_vocab)) return "";
                scanString(str2,is_vocab);
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
private:
    void scanString(std::string& str, bool is_vocab) {
        if (!support_get_mode) return;
        if (get_string=="") {
            if (get_mode && get_string=="") {
                get_string = str;
                get_is_vocab = is_vocab;
            } else if (str=="get") {
                get_mode = true;
            } else {
                get_string = "alt";
            }
        }
    }
};

class yarp::os::WireWriter {
private:
    bool get_mode;
    std::string get_string;
    bool get_is_vocab;
    bool need_ok;
public:
    ConnectionWriter& writer;

    WireWriter(ConnectionWriter& writer) : writer(writer) {
        get_mode = get_is_vocab = false;
        need_ok = false;
        writer.convertTextMode();
    }

    WireWriter(WireReader& reader) : writer(reader.getWriter()) {
        get_is_vocab = false;
        need_ok = false;
        writer.convertTextMode();
        get_mode = reader.get_mode;
        if (get_mode) {
            get_string = reader.get_string;
            get_is_vocab = reader.get_is_vocab;
        }
    }

    ~WireWriter() {
        if (need_ok) {
            writeBool(true);
        }
    }

    bool write(WirePortable& obj) {
        return obj.write(*this);
    }

    //bool write(PortWriter& obj) {
    //return obj.write(writer);
    //}

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
                if (bit.length()<=4) {
                    writer.appendInt(BOTTLE_TAG_VOCAB);
                    writer.appendInt(Vocab::encode(bit));
                } else {
                    writeString(bit.c_str());
                }
                bit = "";
            } else {
                bit += ch;
            }
        }
        return true;
    }

    bool writeString(const std::string& tag) {
        writer.appendInt(BOTTLE_TAG_STRING);
        writer.appendInt((int)tag.length()+1);
        writer.appendString(tag.c_str(),'\0');
        return !writer.isError();
    }

    bool writeListHeader(int len) {
        writer.appendInt(BOTTLE_TAG_LIST);
        if (get_mode) {
            writer.appendInt(len+3);
            writer.appendInt(BOTTLE_TAG_VOCAB);
            writer.appendInt(VOCAB2('i','s'));
            if (get_is_vocab) {
                writer.appendInt(BOTTLE_TAG_VOCAB);
                writer.appendInt(Vocab::encode(get_string.c_str()));
            } else {
                writeString(get_string);
            }
            need_ok = true;
        } else {
            writer.appendInt(len);
        }
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

class yarp::os::WireLink {
private:
    yarp::os::Port *port;
    bool replies;
public:
    WireLink() { port = 0/*NULL*/; replies = true; }

    bool isValid() const { return port!=0/*NULL*/; }

    bool attach(yarp::os::Port& port, bool replies) {
        this->port = &port;
        this->replies = replies;
        return true;
    }

    bool write(PortWriter& writer) {
        if (!isValid()) return false;
        return port->write(writer);
    }
    
    bool write(PortWriter& writer, PortReader& reader) {
        if (!isValid()) return false;
        if (!replies) { port->write(writer); return false; }
        return port->write(writer,reader);
    }

    bool stream() { 
        replies = false;
        return true;
    }

    bool query() { 
        replies = true;
        return true;
    }
};


class yarp::os::Wire : public PortReader {
private:
    WireLink _link;
public:
    bool serve(yarp::os::Port& port) {
        _link.attach(port,true);
        port.setReader(*this);
        return true;
    }

    bool stream(yarp::os::Port& port) {
        _link.attach(port,false);
        return true;
    }

    bool query(yarp::os::Port& port) {
        _link.attach(port,true);
        return true;
    }

    WireLink& link() { return _link; }
};


#endif

