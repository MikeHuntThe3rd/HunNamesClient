#include "CurlReq.hpp"

namespace CurlReq {
void setup() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
}
void disconnect() {
  curl_easy_cleanup(curl);
  curl_global_cleanup();
}

size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                     std::string *data) {
  size_t total_size = size * nmemb;
  data->append((char *)contents, total_size);
  return total_size;
}

void wait() {
  auto now = std::chrono::steady_clock::now();
  auto phase =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start) %
      interval;
  auto actual_pause_time = interval - phase;
  std::this_thread::sleep_until(now + actual_pause_time);
}

void SETcurlData(std::string url, std::vector<std::string> headers) {
  struct curl_slist *headr = NULL;
  for (std::string header : headers) {
    headr = curl_slist_append(headr, header.c_str());
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headr);
}

json __GET(std::string url, std::vector<std::string> headers) {
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
//   SETcurlData(https, headers);
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
}

// Connection
Connection::Connection() {
  worker = std::thread([this] { QueueUnloading(); });
}
Connection::~Connection() {
  cv.notify_one();
  worker.join();
}
void Connection::QueueUnloading() {
  Task task;
  while (true) {
    std::unique_lock<std::mutex> lock(thread_lock);
    cv.wait(lock, [&] { return !tasks.empty();});
    task = std::move(tasks.front());
    tasks.pop();
    cv.notify_all();
    json result = task.work();
    task.promise.set_value(result);
    CurlReq::wait();
  }
}

std::future<json> Connection::Add(std::function<json()> f) {
  Task task;
  task.work = std::move(f);
  std::future<json> future = task.promise.get_future();
  {
    std::lock_guard<std::mutex> lock(thread_lock);
    tasks.push(std::move(task));
  }
  cv.notify_one();
  return future;
}
void Connection::WaitUntilQueueEmpty() {
  std::unique_lock<std::mutex> lock(thread_lock);
  cv.wait(lock, [&] { return tasks.empty(); });
}
} // namespace CurlReq