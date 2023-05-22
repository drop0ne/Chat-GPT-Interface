#include <iostream>
#include <string>
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

// Class representing the ChatGPT plugin
class ChatGPTPlugin {
public:
    // Constructor that initializes the plugin with the API key and ChatGPT endpoint
    ChatGPTPlugin(const std::string& apiKey, const std::string& chatGptEndpoint)
        : apiKey(apiKey), chatGptEndpoint(chatGptEndpoint) {
    }

    // Method to authenticate and obtain the access token
    bool authenticate() {
        // Initialize a curl object for making HTTP requests
        CURL* curl = curl_easy_init();
        if (curl) {
            // Set the request URL for obtaining the access token
            std::string requestUrl = "https://api.cognitive.microsoft.com/sts/v1.0/issueToken";
            curl_easy_setopt(curl, CURLOPT_URL, requestUrl.c_str());

            // Set the API key as a header for authentication
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, ("Ocp-Apim-Subscription-Key: " + apiKey).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            // Perform the request and store the response in a string
            std::string response;
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "Authentication failed: " << curl_easy_strerror(res) << std::endl;
                return false;
            }

            // Clean up and free the resources
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            // Store the obtained access token
            accessToken = response;
            return true;
        }
        return false;
    }

    // Method to send a prompt to ChatGPT and receive the response
    std::string sendPrompt(const std::string& prompt) {
        // Check if an access token is available
        if (accessToken.empty()) {
            std::cerr << "Access token is missing. Please authenticate first." << std::endl;
            return "";
        }

        // Initialize a curl object for making HTTP requests
        CURL* curl = curl_easy_init();
        if (curl) {
            // Set the request URL for sending prompts to ChatGPT
            std::string requestUrl = chatGptEndpoint + "/v1/engines/davinci/completions";
            curl_easy_setopt(curl, CURLOPT_URL, requestUrl.c_str());

            // Set the headers for authentication and content type
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            // Prepare the JSON data for the prompt
            rapidjson::Document jsonDoc;
            jsonDoc.SetObject();
            rapidjson::Value promptValue(prompt.c_str(), jsonDoc.GetAllocator());
            jsonDoc.AddMember("prompt", promptValue, jsonDoc.GetAllocator());

            // Serialize the JSON data to a string
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            jsonDoc.Accept(writer);
            std::string jsonData = buffer.GetString();

            // Set the request body with the prompt JSON data
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());

            // Perform the request and store the response in a string
            std::string response;
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
                return "";
            }

            // Clean up and free the resources
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            // Return the ChatGPT response
            return response;
        }
        return "";
    }

private:
    std::string apiKey;             // API key for authentication
    std::string chatGptEndpoint;    // ChatGPT endpoint URL
    std::string accessToken;        // Access token obtained through authentication

    // Callback function to receive the response data
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
        size_t totalSize = size * nmemb;
        response->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }
};

int main() {
    std::string apiKey = "YOUR_API_KEY";
    std::string chatGptEndpoint = "https://your-chatgpt-endpoint/";

    // Create an instance of the ChatGPTPlugin
    ChatGPTPlugin chatGptPlugin(apiKey, chatGptEndpoint);

    // Authenticate and obtain the access token
    if (!chatGptPlugin.authenticate()) {
        std::cerr << "Failed to authenticate." << std::endl;
        return 1;
    }

    // Send a prompt to ChatGPT and receive the response
    std::string prompt = "Hello, ChatGPT!";
    std::string response = chatGptPlugin.sendPrompt(prompt);
    if (response.empty()) {
        std::cerr << "Failed to get response from ChatGPT." << std::endl;
        return 1;
    }

    // Display the ChatGPT response
    std::cout << "ChatGPT response: " << response << std::endl;

    return 0;
}
