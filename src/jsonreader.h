#ifndef JSONREADER_H
#define JSONREADER_H

#include <json-glib/json-glib.h>
#include <QDebug>

namespace Json {

class Reader {

private:
    long mainIt = 0;
    long internalIt = 0;

    JsonParser *parser = nullptr;
    JsonReader *reader = nullptr;

    GError *err = nullptr;

public:
    enum VType {
        Int,
        Double,
        String,
        Boolean
    };

    struct Value {
        int _int;
        double _double;
        std::string _string;
        bool _boolean;
    };

    Reader(std::string);
    ~Reader();

    template<typename T>
    void fetch(const T *t) {
        json_reader_read_member(reader, t);
        ++mainIt;
    }

    template<typename First, typename ...Args>
    void fetch(const First *first, const Args *...args) {
        json_reader_read_member(reader, first);
        fetch(args...);
        ++mainIt;
    }

    Value getValue(VType type) {
        Value _value;

        switch (type) {
        case VType::Int:
            _value._int = json_reader_get_int_value(reader);
            break;
        case VType::Double:
            _value._double = json_reader_get_double_value(reader);
            break;
        case VType::String:
            _value._string = json_reader_get_string_value(reader);
            break;
        case VType::Boolean:
            _value._boolean = json_reader_get_boolean_value(reader);
            break;
        }

        end();

        return _value;
    }

    Value searchElement(std::string, std::string, int);
    void end();

};

}

#endif
