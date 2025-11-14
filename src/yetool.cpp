#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>
#include <random>
#include <sstream>
#include <regex>
#include <map>
#include <vector>

using namespace std;

// ==================== Utility Functions ====================

// HTTP响应回调
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// 生成36进制随机字符串
string gen36String(int length = 32) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 35);
    
    const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    string result;
    
    for (int i = 0; i < length; ++i) {
        result += charset[dis(gen)];
    }
    return result;
}

// Base64编码
string base64Encode(const string& input) {
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    string ret;
    int val = 0;
    int valb = 6;
    
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 6) {
            valb -= 6;
            ret.push_back(base64_chars[(val >> valb) & 0x3F]);
        }
    }
    
    if (valb > 0) {
        ret.push_back(base64_chars[(val << (6 - valb)) & 0x3F]);
    }
    
    while (ret.size() % 4) {
        ret.push_back('=');
    }
    
    return ret;
}

// Base64解码
string base64Decode(const string& encoded) {
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) {
        T[base64_chars[i]] = i;
    }
    
    string ret;
    int val = 0;
    int valb = 0;
    
    for (unsigned char c : encoded) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 8) {
            valb -= 8;
            ret.push_back(char((val >> valb) & 0xFF));
        }
    }
    
    return ret;
}

// 从URL中提取查询参数
string getUrlParam(const string& url, const string& key) {
    size_t paramPos = url.find('?');
    if (paramPos == string::npos) return "";
    
    string params = url.substr(paramPos + 1);
    size_t keyPos = params.find(key + "=");
    
    if (keyPos == string::npos) return "";
    
    size_t valueStart = keyPos + key.length() + 1;
    size_t valueEnd = params.find('&', valueStart);
    
    if (valueEnd == string::npos) {
        return params.substr(valueStart);
    }
    
    return params.substr(valueStart, valueEnd - valueStart);
}

// ==================== YeTool Class ====================

class YeTool {
private:
    string shareKey;
    string password;
    map<string, string> headers;
    
    // API URLs
    static constexpr const char* SHARE_URL_PREFIX = "https://www.123pan.com/s/";
    static constexpr const char* GET_FILE_INFO_URL = 
        "https://www.123pan.com/a/api/share/get?limit=100&next=1&orderBy=file_name"
        "&orderDirection=asc&shareKey={shareKey}&SharePwd={pwd}&ParentFileId={ParentFileId}"
        "&Page=1&event=homeListFile&operateType=1";
    static constexpr const char* DOWNLOAD_API_URL = 
        "https://www.123pan.com/a/api/share/download/info?{authK}={authV}";
    static constexpr const char* BATCH_DOWNLOAD_API_URL = 
        "https://www.123pan.com/b/api/file/batch_download_share_info?{authK}={authV}";
    
    // 初始化HTTP头
    void initializeHeaders() {
        headers["Accept-Language"] = "zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6";
        headers["App-Version"] = "3";
        headers["Cache-Control"] = "no-cache";
        headers["Connection"] = "keep-alive";
        headers["LoginUuid"] = gen36String();
        headers["Pragma"] = "no-cache";
        headers["Referer"] = string(SHARE_URL_PREFIX) + shareKey + ".html";
        headers["Sec-Fetch-Dest"] = "empty";
        headers["Sec-Fetch-Mode"] = "cors";
        headers["Sec-Fetch-Site"] = "same-origin";
        headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/127.0.0.0 Safari/537.36 Edg/127.0.0.0";
        headers["platform"] = "web";
        headers["sec-ch-ua"] = "\"Not)A;Brand\";v=\"99\", \"Microsoft Edge\";v=\"127\", \"Chromium\";v=\"127\"";
        headers["sec-ch-ua-mobile"] = "?0";
        headers["sec-ch-ua-platform"] = "Windows";
    }
    
    // HTTP GET请求
    string httpGet(const string& url) {
        CURL* curl = curl_easy_init();
        if (!curl) return "";
        
        string response;
        struct curl_slist* headers_list = nullptr;
        
        for (const auto& header : headers) {
            string h = header.first + ": " + header.second;
            headers_list = curl_slist_append(headers_list, h.c_str());
        }
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        CURLcode res = curl_easy_perform(curl);
        
        curl_slist_free_all(headers_list);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            return "";
        }
        
        return response;
    }
    
    // HTTP POST请求
    string httpPost(const string& url, const Json::Value& jsonData) {
        CURL* curl = curl_easy_init();
        if (!curl) return "";
        
        string response;
        struct curl_slist* headers_list = nullptr;
        
        headers_list = curl_slist_append(headers_list, "Content-Type: application/json");
        for (const auto& header : headers) {
            string h = header.first + ": " + header.second;
            headers_list = curl_slist_append(headers_list, h.c_str());
        }
        
        Json::StreamWriterBuilder writer;
        string postData = Json::writeString(writer, jsonData);
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        CURLcode res = curl_easy_perform(curl);
        
        curl_slist_free_all(headers_list);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            return "";
        }
        
        return response;
    }
    
    // 构建URL（替换占位符）
    string buildUrl(const string& baseUrl, const map<string, string>& params) {
        string url = baseUrl;
        
        for (const auto& param : params) {
            string placeholder = "{" + param.first + "}";
            size_t pos = url.find(placeholder);
            if (pos != string::npos) {
                url.replace(pos, placeholder.length(), param.second);
            }
        }
        
        return url;
    }
    
    // 获取签名（需要调用外部YeTool.h）
    pair<string, string> getSignature(const string& apiPath) {
        // 调用外部签名函数：getSign(apiPath)
        // 返回 {authKey, authValue}
        // 这里返回默认值，实际应由签名函数计算
        return {"s", ""};
    }
    
    // 获取文件下载链接
    string getDownUrl(const Json::Value& fileInfo) {
        // 构建请求体
        Json::Value requestBody;
        requestBody["ShareKey"] = fileInfo.get("ShareKey", "");
        requestBody["FileID"] = fileInfo.get("FileId", 0);
        requestBody["S3keyFlag"] = fileInfo.get("S3KeyFlag", "");
        requestBody["Size"] = fileInfo.get("Size", 0);
        requestBody["Etag"] = fileInfo.get("Etag", "");
        
        // 获取签名
        auto signature = getSignature("/a/api/share/download/info");
        
        // 构建下载API URL
        string downloadUrl = DOWNLOAD_API_URL;
        map<string, string> urlParams;
        urlParams["authK"] = signature.first;
        urlParams["authV"] = signature.second;
        downloadUrl = buildUrl(downloadUrl, urlParams);
        
        // 发送POST请求
        string response = httpPost(downloadUrl, requestBody);
        
        Json::CharReaderBuilder reader;
        Json::Value responseJson;
        string errs;
        
        // 修复：创建具名的 istringstream 对象
        istringstream iss1(response);
        if (!Json::parseFromStream(reader, iss1, &responseJson, &errs)) {
            cerr << "Failed to parse download response: " << errs << endl;
            return "";
        }
        
        if (responseJson.get("code", -1).asInt() != 0) {
            cerr << "Download API returned error" << endl;
            return "";
        }
        
        // 获取下载URL
        string downloadUrlEncoded = responseJson["data"].get("DownloadURL", "").asString();
        
        if (downloadUrlEncoded.empty()) {
            cerr << "DownloadURL not found" << endl;
            return "";
        }
        
        // 解析params参数
        string params = getUrlParam(downloadUrlEncoded, "params");
        if (params.empty()) {
            return downloadUrlEncoded;
        }
        
        // Base64解码
        string decodedUrl = base64Decode(params);
        
        // 获取最终的直链
        string finalResponse = httpGet(decodedUrl);
        
        Json::CharReaderBuilder finalReader;
        Json::Value finalJson;
        
        // 修复：创建具名的 istringstream 对象
        istringstream iss2(finalResponse);
        if (!Json::parseFromStream(finalReader, iss2, &finalJson, &errs)) {
            cerr << "Failed to parse final response: " << errs << endl;
            return "";
        }
        
        return finalJson["data"].get("redirect_url", "").asString();
    }
    
    // 获取ZIP文件下载链接（目录）
    string getZipDownUrl(const Json::Value& fileInfo) {
        // 构建请求体
        Json::Value requestBody;
        requestBody["ShareKey"] = fileInfo.get("ShareKey", "");
        
        Json::Value fileIdList(Json::arrayValue);
        Json::Value fileIdObj;
        fileIdObj["fileId"] = fileInfo.get("FileId", 0);
        fileIdList.append(fileIdObj);
        
        requestBody["fileIdList"] = fileIdList;
        
        // 获取签名
        auto signature = getSignature("/a/api/file/batch_download_share_info");
        
        // 构建批量下载API URL
        string batchDownloadUrl = BATCH_DOWNLOAD_API_URL;
        map<string, string> urlParams;
        urlParams["authK"] = signature.first;
        urlParams["authV"] = signature.second;
        batchDownloadUrl = buildUrl(batchDownloadUrl, urlParams);
        
        // 发送POST请求
        string response = httpPost(batchDownloadUrl, requestBody);
        
        Json::CharReaderBuilder reader;
        Json::Value responseJson;
        string errs;
        
        // 修复：创建具名的 istringstream 对象
        istringstream iss3(response);
        if (!Json::parseFromStream(reader, iss3, &responseJson, &errs)) {
            cerr << "Failed to parse batch download response: " << errs << endl;
            return "";
        }
        
        if (responseJson.get("code", -1).asInt() != 0) {
            cerr << "Batch download API returned error" << endl;
            return "";
        }
        
        return responseJson["data"].get("DownloadUrl", "").asString();
    }
    
public:
    // 构造函数
    YeTool(const string& key, const string& pwd = "") 
        : shareKey(key), password(pwd) {
        initializeHeaders();
    }
    
    // 解析分享链接获取下载地址
    string parse() {
        // 清理shareKey
        string cleanShareKey = shareKey;
        size_t pos = cleanShareKey.find_first_of(".#");
        if (pos != string::npos) {
            cleanShareKey = cleanShareKey.substr(0, pos);
        }
        
        // 构建URL
        map<string, string> urlParams;
        urlParams["shareKey"] = cleanShareKey;
        urlParams["pwd"] = password;
        urlParams["ParentFileId"] = "0";
        
        string getFileInfoUrl = buildUrl(GET_FILE_INFO_URL, urlParams);
        
        // 发送请求
        string response = httpGet(getFileInfoUrl);
        
        Json::CharReaderBuilder reader;
        Json::Value responseJson;
        string errs;
        
        // 修复：创建具名的 istringstream 对象
        istringstream iss4(response);
        if (!Json::parseFromStream(reader, iss4, &responseJson, &errs)) {
            cerr << "Failed to parse JSON response: " << errs << endl;
            return "";
        }
        
        // 检查响应状态
        if (responseJson.get("code", -1).asInt() != 0) {
            cerr << "API returned error code: " << responseJson.get("code", -1).asInt() << endl;
            return "";
        }
        
        // 获取文件信息
        Json::Value data = responseJson.get("data", Json::Value());
        Json::Value infoList = data.get("InfoList", Json::Value());
        
        if (infoList.empty()) {
            cerr << "InfoList is empty" << endl;
            return "";
        }
        
        Json::Value fileInfo = infoList[0];
        fileInfo["ShareKey"] = cleanShareKey;
        
        // 检查是否为文件夹 (Type: 1为文件夹, 0为文件)
        int type = fileInfo.get("Type", 0).asInt();
        
        if (type == 1) {
            return getZipDownUrl(fileInfo);
        } else {
            return getDownUrl(fileInfo);
        }
    }
    
    // 获取文件列表
    Json::Value parseFileList(const string& parentFileId = "0") {
        // 清理shareKey
        string cleanShareKey = shareKey;
        size_t pos = cleanShareKey.find_first_of(".#");
        if (pos != string::npos) {
            cleanShareKey = cleanShareKey.substr(0, pos);
        }
        
        // 构建URL
        map<string, string> urlParams;
        urlParams["shareKey"] = cleanShareKey;
        urlParams["pwd"] = password;
        urlParams["ParentFileId"] = parentFileId;
        
        string getFileInfoUrl = buildUrl(GET_FILE_INFO_URL, urlParams);
        
        // 发送请求
        string response = httpGet(getFileInfoUrl);
        
        Json::CharReaderBuilder reader;
        Json::Value responseJson;
        string errs;
        
        // 修复：创建具名的 istringstream 对象
        istringstream iss5(response);
        if (!Json::parseFromStream(reader, iss5, &responseJson, &errs)) {
            cerr << "Failed to parse file list response: " << errs << endl;
            return Json::Value();
        }
        
        if (responseJson.get("code", -1).asInt() != 0) {
            cerr << "File list API returned error" << endl;
            return Json::Value();
        }
        
        return responseJson["data"].get("InfoList", Json::Value());
    }
    
    // 根据FileID获取下载链接
    string parseById(const string& fileId) {
        // 获取文件详情
        Json::Value fileList = parseFileList("0");
        
        if (fileList.empty()) {
            cerr << "Failed to get file list" << endl;
            return "";
        }
        
        // 查找指定FileID的文件
        for (unsigned int i = 0; i < fileList.size(); ++i) {
            if (fileList[i].get("FileId", "").asString() == fileId) {
                return getDownUrl(fileList[i]);
            }
        }
        
        cerr << "File ID not found: " << fileId << endl;
        return "";
    }
    
    // 设置自定义HTTP头
    void setHeader(const string& name, const string& value) {
        headers[name] = value;
    }
};

// ==================== Main ====================

int main() {
    // 示例：解析单个文件
    string shareKey = "iaKtVv-6OECd";  // 替换为实际的分享KEY
    string password = "";              // 替换为分享密码（如果需要）
    
    YeTool yeTool(shareKey, password);
    
    // 获取下载链接
    cout << "开始解析123网盘分享链接..." << endl;
    string downloadUrl = yeTool.parse();
    
    if (!downloadUrl.empty()) {
        cout << "下载链接: " << downloadUrl << endl;
    } else {
        cout << "解析失败" << endl;
    }
    
    // 示例：获取文件列表
    cout << "\n获取文件列表..." << endl;
    Json::Value fileList = yeTool.parseFileList("0");
    
    cout << "文件列表:" << endl;
    cout << fileList.toStyledString() << endl;
    
    return 0;
}