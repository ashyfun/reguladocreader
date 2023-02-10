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
    enum MemberType {
        Int,
        Double,
        String,
        Bool
    };

    struct MemberValue {
        int mvInt;
        double mvDouble;
        std::string mvString;
        bool mvBool;
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

    MemberValue getValue(MemberType type) {
        MemberValue v;

        switch (type) {
        case MemberType::Int:
            v.mvInt = json_reader_get_int_value(reader);
            break;
        case MemberType::Double:
            v.mvDouble = json_reader_get_double_value(reader);
            break;
        case MemberType::String:
            v.mvString = json_reader_get_string_value(reader);
            break;
        case MemberType::Bool:
            v.mvBool = json_reader_get_boolean_value(reader);
            break;
        default:
            qDebug() << "Member type is unknown:" << type;
            break;
        }

        end();
        return v;
    }

    MemberValue searchElement(std::string, std::string, int);
    void end();

};

}

#endif
