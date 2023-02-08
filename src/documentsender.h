#ifndef DOCUMENTSENDER_H
#define DOCUMENTSENDER_H

#include <QDebug>
#include <curl/curl.h>
#include <iostream>
#include <vector>
#include <string>

class DocumentSender {

private:
    CURL *curl = nullptr;

    struct curl_slist *headers = nullptr;

    struct Mime {
        std::string name;
        std::string value;
        bool isFile;
    };
    std::vector<Mime> preparedMime;

    curl_mime *mime = nullptr;
    curl_mimepart *part = nullptr;

    void setUpMime();

public:
    DocumentSender();
    ~DocumentSender();

    void setHeaders(std::vector<std::string> &);
    void addMimePart(std::string, std::string, bool = false);
    void doPost(std::string);

};

#endif
