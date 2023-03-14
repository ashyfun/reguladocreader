#include "documentsender.h"

DocumentSender::DocumentSender() {
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    }
}

DocumentSender::~DocumentSender() {
    curl_mime_free(mime);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void DocumentSender::setUpMime() {
    if (mime) {
        curl_mime_free(mime);
    }

    mime = curl_mime_init(curl);
    for (auto m : preparedMime) {
        part = curl_mime_addpart(mime);
        curl_mime_name(part, m.name.c_str());

        if (m.isFile) {
            curl_mime_filedata(part, m.value.c_str());
        } else {
            curl_mime_data(part, m.value.c_str(), CURL_ZERO_TERMINATED);
        }
    }
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
}

void DocumentSender::setHeaders(std::vector<std::string> &h) {
    if (headers) {
        curl_slist_free_all(headers);
    }

    for (auto value : h) {
        curl_slist_append(headers, value.c_str());
    }
}

void DocumentSender::addMimePart(std::string name, std::string value, bool isFile) {
    preparedMime.push_back(Mime{ name, value, isFile });
}

bool DocumentSender::mimeIsExist(std::string name)
{
    bool isExist = false;
    for (auto mime : preparedMime)
    {
        if (mime.name == name)
        {
            isExist = true;
            break;
        }
    }

    return isExist;
}

unsigned DocumentSender::howManyMimeParts() {
    return preparedMime.size();
}

void DocumentSender::doPost(std::string url) {
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        setUpMime();

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            qDebug() << "curl_easy_perform() failed:" << curl_easy_strerror(res) << "\n";
        }

        preparedMime.clear();
    }
}
