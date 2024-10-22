#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>
#include <random>
#include <sstream>
void getDownURL(const std::string& key, const std::string& fileId);

// 生成UUID
std::string generateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::stringstream ss;

    ss << std::hex;
    for (int i = 0; i < 8; ++i) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; ++i) ss << dis(gen);
    ss << "-4";  // UUID version 4
    for (int i = 0; i < 3; ++i) ss << dis(gen);
    ss << "-";
    ss << dis(gen) % 4 + 8;  // variant 1
    for (int i = 0; i < 3; ++i) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; ++i) ss << dis(gen);
    
    return ss.str();
}

// HTTP 响应处理回调函数
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}

// 发起 HTTP POST 请求
std::string httpPost(const std::string& url, const Json::Value& jsonData) {
    CURL* curl;
    CURLcode res;
    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    Json::StreamWriterBuilder writer;
    std::string postData = Json::writeString(writer, jsonData);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // 执行请求
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    return response;
}

// 第一步解析请求：解析分享信息
void parse(const std::string& shareKey, const std::string& sharePassword) {
    const std::string API_URL_PREFIX = "https://lecloud.lenovo.com/share/api/clouddiskapi/share/public/v1/";
    const std::string apiUrl1 = API_URL_PREFIX + "shareInfo";

    // 构造 JSON 请求
    Json::Value requestJson;
    requestJson["shareId"] = shareKey;
    requestJson["password"] = sharePassword;
    requestJson["directoryId"] = -1;

    // 发送 POST 请求
    std::string response = httpPost(apiUrl1, requestJson);
    Json::Reader reader;
    Json::Value root;

    if (reader.parse(response, root)) {
        if (root.isMember("result") && root["result"].asBool()) {
            Json::Value dataJson = root["data"];
            if (!dataJson["passwordVerified"].asBool()) {
                std::cerr << "密码验证失败, 分享key: " << shareKey << ", 密码: " << sharePassword << std::endl;
                return;
            }

            // 获取文件信息
            Json::Value files = dataJson["files"];
            if (files.empty()) {
                std::cerr << "Result JSON数据异常: files字段不存在或jsonArray长度为空" << std::endl;
                return;
            }

            Json::Value fileInfoJson = files[0];
            std::string fileId = fileInfoJson["fileId"].asString();
            getDownURL(shareKey, fileId);
        } else {
            std::cerr << root["errcode"].asString() << ": " << root["errmsg"].asString() << std::endl;
        }
    } else {
        std::cerr << "Failed to parse JSON response from shareInfo." << std::endl;
    }
}

// 第二步：获取下载链接
void getDownURL(const std::string& key, const std::string& fileId) {
    const std::string API_URL_PREFIX = "https://lecloud.lenovo.com/share/api/clouddiskapi/share/public/v1/";
    const std::string apiUrl2 = API_URL_PREFIX + "packageDownloadWithFileIds";

    // 生成 UUID
    std::string uuid = generateUUID();

    // 构造 JSON 请求
    Json::Value requestJson;
    Json::Value fileIds(Json::arrayValue);
    fileIds.append(fileId);
    requestJson["fileIds"] = fileIds;
    requestJson["shareId"] = key;
    requestJson["browserId"] = uuid;

    // 发送 POST 请求
    std::string response = httpPost(apiUrl2, requestJson);
    Json::Reader reader;
    Json::Value root;

    if (reader.parse(response, root)) {
        if (root.isMember("result") && root["result"].asBool()) {
            Json::Value dataJson = root["data"];
            std::string downloadUrl = dataJson["downloadUrl"].asString();
            if (downloadUrl.empty()) {
                std::cerr << "Result JSON数据异常: downloadUrl不存在" << std::endl;
                return;
            }

            std::cout << "重定向链接: " << downloadUrl << std::endl;
            // 这里可以进一步处理下载链接，例如重定向
        } else {
            std::cerr << root["errcode"].asString() << ": " << root["errmsg"].asString() << std::endl;
        }
    } else {
        std::cerr << "Failed to parse JSON response from packageDownloadWithFileIds." << std::endl;
    }
}

// https://lecloud.lenovo.com/share/48N3h8XEUvPi9xjcF

int main() {
    std::string shareKey = "48N3h8XEUvPi9xjcF";  // 替换为实际的分享Key
    std::string sharePassword = "";  // 替换为实际的密码
    parse(shareKey, sharePassword);
    return 0;
}
