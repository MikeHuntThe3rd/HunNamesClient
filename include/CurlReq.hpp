#pragma once

#include "expected.hpp"
#include "json.hpp"
#include "types.hpp"
#include <chrono>
#include <condition_variable>
#include <curl/curl.h>
#include <future>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using json = nlohmann::json;
namespace CurlReq {

class Connection {
public:
  Connection();
  ~Connection();
  std::future<tl::expected<json, GETERR>>
  GET(std::string base_url, const std::vector<std::string> &headers,
      const std::vector<std::pair<std::string, std::string>> &parameters);

private:
  CURL *curl = nullptr;
  bool stop = false;
  struct Task {
    std::function<tl::expected<json, GETERR>()> work;
    std::promise<tl::expected<json, GETERR>> promise;

    Task(std::function<tl::expected<json, GETERR>()> work,
         std::promise<tl::expected<json, GETERR>> promise)
        : work(work), promise(std::move(promise)) {};
  };

  std::chrono::steady_clock::time_point start{std::chrono::steady_clock::now()};
  std::chrono::milliseconds interval{1000};
  std::string response_string;
  std::queue<Task> tasks;
  std::mutex thread_lock;
  std::condition_variable cv;
  std::thread worker;

  static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                              std::string *data) {
    size_t total_size = size * nmemb;
    data->append((char *)contents, total_size);
    return total_size;
  }
  curl_slist *set_http_headers(const std::vector<std::string> &headers);
  void set_url_parameters(
      std::string &url,
      const std::vector<std::pair<std::string, std::string>> &parameters);

  void setup();
  void disconnect();
  void wait();
  void QueueUnloading();
};
} // namespace CurlReq
