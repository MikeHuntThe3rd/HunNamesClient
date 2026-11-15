#include "CurlReq.hpp"
#include <curl/curl.h>
#include <string>
#include <winscard.h>

namespace CurlReq {
void Connection::setup() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
}
void Connection::disconnect() {
  curl_easy_cleanup(curl);
  curl_global_cleanup();
}

void Connection::wait() {
  auto now = std::chrono::steady_clock::now();
  auto phase =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start) %
      interval;
  auto actual_pause_time = interval - phase;
  std::this_thread::sleep_until(now + actual_pause_time);
}

void Connection::set_http_headers(const std::vector<std::string> &headers) {
  struct curl_slist *header_list = NULL;
  for (std::string header : headers) {
    header_list = curl_slist_append(header_list, header.c_str());
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
}

void set_url_parameters(std::string &url, const std::vector<std::pair<std::string, std::string>> &parameters) {
  if (!parameters.empty()) {
    url += "?";
  }

  bool first = true;
  for(const auto &parameter : parameters) {
    if (first) first = false;
    else url += "&";

    url += parameter.first + "=" + parameter.second;
  }
}


std::future<tl::expected<json, int>> Connection::GET(std::string base_url, const std::vector<std::string> &headers, const std::vector<std::pair<std::string, std::string>> &parameters) {
  std::function<tl::expected<json, int>()> fn = [&]() -> tl::expected<json, int> {
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    set_url_parameters(base_url, parameters);
    curl_easy_setopt(curl, CURLOPT_URL, base_url.c_str());
    set_http_headers(headers);

    CURLcode response = curl_easy_perform(curl);

    if (response != CURLE_OK) {
        // std::cerr << "GET failed: " << curl_easy_strerror(response) << std::endl;
        response_string.clear();
        return json{};
    } else if (response_string.length() == 0) {
        return json{};
    } else {
        try {
        json data = json::parse(response_string);
        response_string.clear();
        return data;
        } catch (nlohmann::json::parse_error &err) {
          return json{};
        }
    }
  };

  std::promise<tl::expected<json, int>> promise;
  std::future<tl::expected<json, int>> future = promise.get_future();
  Task task{ fn, std::move(promise) };
  {
    std::lock_guard<std::mutex> lock(thread_lock);
    tasks.push(std::move(task));
  }
  cv.notify_one();
  
  return future;
}

Connection::Connection() {
  worker = std::thread([this] { QueueUnloading(); });
}
Connection::~Connection() {
  disconnect();
  WaitUntilQueueEmpty();
  cv.notify_one();
  worker.join();
}

void Connection::QueueUnloading() {
  while (true) {
    std::unique_lock<std::mutex> lock(thread_lock);
    cv.wait(lock, [&] { return !tasks.empty();});
    Task task = std::move(tasks.front());
    tasks.pop();
    auto result = task.work();
    task.promise.set_value(result);
    cv.notify_all();
    wait();
  }
}

void Connection::WaitUntilQueueEmpty() {
  std::unique_lock<std::mutex> lock(thread_lock);
  cv.wait(lock, [&] { return tasks.empty(); });
}

} // namespace CurlReq