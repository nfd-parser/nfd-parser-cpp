#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>

// 解析响应数据的回调函数
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}

// 用于发起 HTTP GET 请求的函数
std::string httpGet(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string response;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        // 执行请求
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    return response;
}

// 解析和处理返回的 JSON 响应
void parse(const std::string& shareKey) {
    const std::string API_REQUEST_URL = "https://cowtransfer.com/core/api/transfer/share";
    std::string url = API_REQUEST_URL + "?uniqueUrl=" + shareKey;

    // 第一次请求
    std::string response = httpGet(url);
    Json::Reader reader;
    Json::Value root;

    if (reader.parse(response, root)) {
        if (root["message"].asString() == "success" && root.isMember("data")) {
            Json::Value data = root["data"];
            std::string guid = data["guid"].asString();
            std::string url2 = API_REQUEST_URL + "/download?transferGuid=" + guid;

            if (data["zipDownload"].asBool()) {
                std::string title = data["firstFolder"]["title"].asString();
                url2 += "&title=" + title;
            } else {
                std::string fileId = data["firstFile"]["id"].asString();
                url2 += "&fileId=" + fileId;
            }

            // 第二次请求
            std::string response2 = httpGet(url2);
            Json::Value root2;
            if (reader.parse(response2, root2)) {
                if (root2["message"].asString() == "success" && root2.isMember("data")) {
                    std::string downloadUrl = root2["data"]["downloadUrl"].asString();
                    if (!downloadUrl.empty()) {
                        std::cout << "Cow parse success: " << downloadUrl << std::endl;
                        return;
                    } else {
                        std::cerr << "Cow parse failed: downloadUrl is empty" << std::endl;
                    }
                } else {
                    std::cerr << "Cow parse failed: " << url2 << "; json: " << root2.toStyledString() << std::endl;
                }
            } else {
                std::cerr << "Failed to parse JSON response from second request." << std::endl;
            }
        } else {
            std::cerr << "Cow parse failed: " << shareKey << "; json: " << root.toStyledString() << std::endl;
        }
    } else {
        std::cerr << "Failed to parse JSON response from first request." << std::endl;
    }
}

int main() {
	// https://cowtransfer.com/s/9bef49c4210443
    std::string shareKey = "9bef49c4210443";  // 替换为实际的分享Key
    parse(shareKey);
    return 0;
}
