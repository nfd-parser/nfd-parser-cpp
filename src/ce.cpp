#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>

class ShareLinkInfo {
public:
    std::string shareKey;
    std::string sharePassword;
    std::string shareUrl;

    ShareLinkInfo(const std::string& key, const std::string& password, const std::string& url)
        : shareKey(key), sharePassword(password), shareUrl(url) {}
};

class PanBase {
protected:
    ShareLinkInfo shareLinkInfo;

public:
    PanBase(const ShareLinkInfo& info) : shareLinkInfo(info) {}
};

class CeTool : public PanBase {
private:
    const std::string DOWNLOAD_API_PATH = "/api/v3/share/download/";
    const std::string SHARE_API_PATH = "/api/v3/share/info/";

public:
    CeTool(const ShareLinkInfo& info) : PanBase(info) {}

    std::string parse() {
        std::string key = shareLinkInfo.shareKey;
        std::string pwd = shareLinkInfo.sharePassword;
        std::string url = shareLinkInfo.shareUrl;

        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "CURL initialization failed" << std::endl;
            return "";
        }

        std::string downloadApiUrl = "https://" + extractHost(url) + DOWNLOAD_API_PATH + key + "?path=undefined/undefined;";
        std::string shareApiUrl = "https://" + extractHost(url) + SHARE_API_PATH + key;

        // Set up the first request to get the share info
        curl_easy_setopt(curl, CURLOPT_URL, shareApiUrl.c_str());
        if (!pwd.empty()) {
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, ("password: " + pwd).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }

        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        // Parse the response and get the download URL
        std::cout << "Response: " << response << std::endl;
        Json::Value root;
        Json::Reader reader;
        if (reader.parse(response, root)) {
            if (root["code"].asInt() == 0) {
                return root["data"].asString();
            } else {
                std::cerr << "JSON parsing failed: " << root.toStyledString() << std::endl;
            }
        } else {
            std::cerr << "Failed to parse JSON: " << reader.getFormattedErrorMessages() << std::endl;
        }

        return "";
    }

private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    std::string extractHost(const std::string& url) {
        size_t start = url.find("://") + 3;
        size_t end = url.find('/', start);
        return url.substr(start, end - start);
    }
};

int main() {
	//https://pan.huang1111.cn/s/g31PcQ
    ShareLinkInfo info("g31PcQ", "qaiu", "https://pan.huang1111.cn");
    CeTool tool(info);
    std::string downloadUrl = tool.parse();
    std::cout << "Download URL: " << downloadUrl << std::endl;
    return 0;
}