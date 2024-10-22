#include <iostream>
#include <curl/curl.h>
#include <regex>
#include <future>
#include <string>
#include <json/json.h> // Assuming a JSON library
#include <map>

using namespace std;

const string SHARE_URL_PREFIX = "https://wwwa.lanzoux.com";

// Function to handle the response body
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to send HTTP GET request
string httpGet(const string& url, const string& referer = "") {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        if (!referer.empty()) {
            curl_easy_setopt(curl, CURLOPT_REFERER, referer.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirects
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}


// Function to send HTTP GET request with headers and retrieve the 'Location' header if it's a redirect
string getFinalLocation(const string& url, const map<string, string>& headers = {}) {
    CURL* curl;
    CURLcode res;
    string finalLocation;

    curl = curl_easy_init();
    if(curl) {
        struct curl_slist* chunk = NULL;
        for (const auto& header : headers) {
            chunk = curl_slist_append(chunk, (header.first + ": " + header.second).c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        // Allow the handling of redirects but don't follow them automatically
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L); // Disable automatic following
        curl_easy_setopt(curl, CURLOPT_HEADER, 1L); // Enable header retrieval
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // We only want the headers

        string headerBuffer;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &headerBuffer);
        res = curl_easy_perform(curl);

        if(res == CURLE_OK) {
            // Parse the response headers to extract 'Location'
            size_t locPos = headerBuffer.find("Location: ");
            if (locPos != string::npos) {
                size_t locEnd = headerBuffer.find("\r\n", locPos);
                finalLocation = headerBuffer.substr(locPos + 10, locEnd - locPos - 10); // Extract URL after "Location: "
            }
        }
        curl_easy_cleanup(curl);
    }

    return finalLocation;
}

// Function to send HTTP POST request with form data
string httpPost(const string& url, const map<string, string>& formFields, const map<string, string>& headers = {}) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        struct curl_slist* chunk = NULL;
        for (const auto& header : headers) {
            chunk = curl_slist_append(chunk, (header.first + ": " + header.second).c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        string postFields;
        for (const auto& field : formFields) {
            if (!postFields.empty()) postFields += "&";
            postFields += field.first + "=" + curl_easy_escape(curl, field.second.c_str(), field.second.length());
        }

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

// Function to extract the data object using regex
map<string, string> extractData(const string& jsText) {
    map<string, string> dataMap;

    // Regular expression to match the entire data object
    regex dataRegex(R"(data\s*:\s*\{([^}]+)\})");
    smatch dataMatch;

    if (regex_search(jsText, dataMatch, dataRegex)) {
        string dataContent = dataMatch[1].str();

        // Split the key-value pairs and store in map
        regex pairRegex(R"(\s*'([^']+)'\s*:\s*'([^']+)')");
        sregex_iterator it(dataContent.begin(), dataContent.end(), pairRegex);
        sregex_iterator end;

        while (it != end) {
            dataMap[it->str(1)] = it->str(2);
            ++it;
        }
    }

    return dataMap;
}

map<string, string> extractSign(const string& html) {
	map<string, string> dataMap;
    // Regex to match the 'data' parameter inside $.ajax
    regex dataRegex(R"(data\s*:\s*\{[^}]*'sign'\s*:\s*(\w+)[^}]*\})");
    smatch dataMatch;

    if (regex_search(html, dataMatch, dataRegex)) {
        string signVariable = dataMatch[1].str();
        cout << "Found sign variable: " << signVariable << endl;

        // Now find the value of this sign variable
        regex signValueRegex(signVariable + R"(\s*=\s*'([^']+)')");
        smatch signValueMatch;
        if (regex_search(html, signValueMatch, signValueRegex)) {
            string signValue = signValueMatch[1].str();
            cout << "Found sign value: " << signValue << endl;
            dataMap["action"] = "downprocess";
            dataMap["sign"] = signValue;
        }
    }
    return dataMap;
}

// Function to get the download URL by parsing the response
string getDownURL(const string& key, const map<string, string>& signMap) {
    // Print signMap for debugging
    cout << "Sign Map:" << endl;
    for (const auto& sign : signMap) {
        cout << sign.first << ": " << sign.second << endl;
    }

    string url = SHARE_URL_PREFIX + "/ajaxm.php";

    map<string, string> headers = {
        {"User-Agent", "Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/111.0.0.0 Mobile Safari/537.36"},
        {"referer", key},
        {"sec-ch-ua-platform", "Android"},
        {"Accept-Language", "zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2"},
        {"sec-ch-ua-mobile", "sec-ch-ua-mobile"}
    };

    string response = httpPost(url, signMap, headers);
    cout << "Response from initial POST request:" << endl << response << endl;

    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();
    Json::Value jsonData;
    string errors;

    if (reader->parse(response.c_str(), response.c_str() + response.size(), &jsonData, &errors)) {
        if (jsonData["zt"].asInt() != 1) {
            cerr << jsonData["inf"].asString() << endl;
            return "";
        }
        string downUrl = jsonData["dom"].asString() + "/file/" + jsonData["url"].asString();
        cout << "Download URL (before final request): " << downUrl << endl;
        
        return getFinalLocation(downUrl, headers); // If no Location header found, return the response
        
    }
    return "";
}


// Main parsing logic
future<string> parse(const string& sUrl, const string& pwd) {
    promise<string> prom;
    future<string> fut = prom.get_future();

    // Send the first GET request
    string html = httpGet(sUrl);
    
    // Match the iframe
    regex iframeRegex("src=\"(/fn\\?[a-zA-Z\\d_+/=]{16,})\"");
    smatch iframeMatch;
    
    if (!regex_search(html, iframeMatch, iframeRegex)) {
        // Handle JS decryption case
        map<string, string> signMap = extractSign(html);
        if (signMap.empty()) {
            prom.set_value("JS script sign match failed, share might be expired");
            return fut;
        }
        // add password
        signMap["p"] = pwd;
        string downloadUrl = getDownURL(sUrl, signMap);
        prom.set_value(downloadUrl);
    } else {
        // No password, handle iframe
        string iframePath = iframeMatch[1].str();
        string iframeHtml = httpGet(SHARE_URL_PREFIX + iframePath);

        map<string, string> signMap = extractData(iframeHtml);
        string downloadUrl = getDownURL(sUrl, signMap);
        prom.set_value(downloadUrl);
    }

    return fut;
}

int main() {
	
	/*
    string shareUrl = "https://lanzoui.com/iDPDC1tod2ni";  // Replace with the actual URL
    string password = "";  // Replace with the actual password
    */
    
	//GET http://127.0.0.1:6400/json/lz/icBp6qqj82b@QAIU
    string shareUrl = "https://lanzoui.com/icBp6qqj82b";  // Replace with the actual URL
    string password = "QAIU";  // Replace with the actual password

    future<string> result = parse(shareUrl, password);
    cout << "Download URL: " << result.get() << endl;

    return 0;
}