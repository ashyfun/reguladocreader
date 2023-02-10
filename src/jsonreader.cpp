#include "jsonreader.h"

namespace Json {

Reader::Reader(std::string data) {
    parser = json_parser_new();
    if (!json_parser_load_from_data(parser, data.c_str(), -1, &err)) {
        qDebug() << "json_parser_load_from_data() failed:" << err->message;
        return;
    }

    reader = json_reader_new(
        json_parser_get_root(parser)
    );
}

Reader::~Reader() {
    if (err) {
        g_error_free(err);
    }

    if (reader) {
        end();
        g_object_unref(reader);
    }

    g_object_unref(parser);
}

Reader::MemberValue Reader::searchElement(std::string byKey, std::string member, int compare) {
    Reader::MemberValue v;

    int countElements = json_reader_count_elements(reader);
    for (int i = 0; i < countElements; i++) {
        json_reader_read_element(reader, i);

        fetch(byKey.c_str());
        internalIt = 1;
        v = getValue(Reader::MemberType::Int);

        if (v.mvInt == compare) {
            fetch(member.c_str());
            internalIt = 1;
            v = getValue(Reader::MemberType::String);

            json_reader_end_element(reader);
            return v;
        }

        json_reader_end_element(reader);
    }

    return v;
}

void Reader::end() {
    if (!reader) {
        return;
    }

    if (internalIt <= 0) {
        internalIt = mainIt;
    }

    while (internalIt) {
        json_reader_end_member(reader);
        --mainIt, --internalIt;
    }
}

}
