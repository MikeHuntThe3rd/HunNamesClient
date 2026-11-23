#include "CurlReq.hpp"
#include "expected.hpp"
#include "types.hpp"
#include <curl/curl.h>
#include <curl/easy.h>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <winscard.h>
#endif

namespace CurlReq {
void Connection::setup() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L);
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

curl_slist *
Connection::set_http_headers(const std::vector<std::string> &headers) {
  curl_slist *header_list = NULL;
  for (std::string header : headers) {
    header_list = curl_slist_append(header_list, header.c_str());
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);

  return header_list;
}

void Connection::set_url_parameters(
    std::string &url,
    const std::vector<std::pair<std::string, std::string>> &parameters) {
  if (!parameters.empty()) {
    url += "?";
  }

  bool first = true;
  for (const auto &parameter : parameters) {
    if (first)
      first = false;
    else
      url += "&";

    url += parameter.first + "=" + parameter.second;
  }
}

std::future<tl::expected<json, GETERR>> Connection::GET(
    std::string base_url, const std::vector<std::string> &headers,
    const std::vector<std::pair<std::string, std::string>> &parameters) {
  std::function<tl::expected<json, GETERR>()> fn =
      [=]() mutable -> tl::expected<json, GETERR> {
    response_string.clear();

    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    set_url_parameters(base_url, parameters);
    curl_easy_setopt(curl, CURLOPT_URL, base_url.c_str());

    auto set_headers = set_http_headers(headers);

    CURLcode response = curl_easy_perform(curl);
    curl_slist_free_all(set_headers);

    switch (response) {
    case CURLcode::CURLE_OK: {
      long status_code;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
      if (status_code != 200) {
        return tl::make_unexpected(GETERR::NON200);
      }

      try {
        json data = json::parse(response_string);
        return data;
      } catch (nlohmann::json::parse_error &err) {
        return tl::make_unexpected(GETERR::FAILEDPARSE);
      }
    }
    default: {
      return tl::make_unexpected(GETERR::CURLFAILURE);
    }
    }
  };

  std::promise<tl::expected<json, GETERR>> promise;
  std::future<tl::expected<json, GETERR>> future = promise.get_future();
  Task task{fn, std::move(promise)};
  {
    std::lock_guard<std::mutex> lock(thread_lock);
    tasks.push(std::move(task));
  }
  cv.notify_one();

  return future;
}

Connection::Connection() {
  setup();
  worker = std::thread([this] { QueueUnloading(); });
}

Connection::~Connection() {
  std::unique_lock<std::mutex> lock(thread_lock);
  stop = true;
  tasks = {};
  lock.unlock();

  cv.notify_all();

  worker.join();
  disconnect();
}

void Connection::QueueUnloading() {
  while (true) {
    std::unique_lock<std::mutex> lock(thread_lock);
    cv.wait(lock, [&] { return stop || !tasks.empty(); });
    if (stop)
      return;

    Task task = std::move(tasks.front());
    tasks.pop();

    lock.unlock();

    auto result = task.work();
    task.promise.set_value(std::move(result));

    wait();
  }
}
} // namespace CurlReq
