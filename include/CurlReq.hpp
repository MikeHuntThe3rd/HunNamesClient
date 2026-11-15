#pragma once

#include <curl/curl.h>
#include "json.hpp"
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;
namespace CurlReq {

class Connection {
public:
  Connection();
  ~Connection();
  std::future<json> GET(std::string base_url, std::vector<std::string> headers = {
                                  "accept: application/json", "Language: en"});
  std::future<json> Add(std::function<json()> f);
  
  private:
    CURL *curl = nullptr;
    std::string response_string;
    std::chrono::steady_clock::time_point start{
    std::chrono::steady_clock::now()};
    std::chrono::milliseconds interval{1000};

    struct Task {
      std::function<json()> work;
      std::promise<json> promise;
    };

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                                std::string *data) {
        size_t total_size = size * nmemb;
        data->append((char *)contents, total_size);
        return total_size;
    }
    void setup();
    void disconnect();
    void wait();
    // void SETcurlData(std::string url, std::vector<std::string> headers);
    
    void WaitUntilQueueEmpty();
    std::queue<Task> tasks;
    std::mutex thread_lock;
    std::condition_variable cv;
    std::thread worker;

    void QueueUnloading();
};

} // namespace CurlReq