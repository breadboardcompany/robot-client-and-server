#pragma once
#include <condition_variable>
#include <mutex>

template <typename T> class Channel {
public:
  Channel() = default;
  Channel(const Channel &) = delete;
  Channel(Channel &&) = delete;
  ~Channel() = default;

  bool closed() { return closed_; }

  void close() {
    closed_ = true;
    cnd_producer_.notify_all();
    cnd_consumer_.notify_all();
  }

  bool send(const T &data) {
    std::unique_lock lk(mtx_);
    cnd_producer_.wait(lk, [this] { return data_ || closed_; });
    if (closed_) {
      return false;
    }

    *data_ = data;
    data_ = nullptr;

    lk.unlock();
    cnd_consumer_.notify_one();
    return true;
  }

  bool send(T &&data) {
    std::unique_lock lk(mtx_);
    cnd_producer_.wait(lk, [this] { return data_ || closed_; });
    if (closed_) {
      return false;
    }

    *data_ = std::move(data);
    data_ = nullptr;

    lk.unlock();
    cnd_consumer_.notify_one();
    return true;
  }

  bool recv(T &data) {
    std::unique_lock lk(mtx_);

    data_ = &data;

    cnd_producer_.notify_one();
    cnd_consumer_.wait(lk, [this] { return !data_ || closed_; });
    if (closed_) {
      return false;
    }

    return true;
  }

private:
  T *data_ = nullptr;
  bool closed_ = false;
  std::mutex mtx_;
  std::condition_variable cnd_producer_;
  std::condition_variable cnd_consumer_;
};
