#pragma once

#include <pico/time.h>

#include <optional>
#include <utility>

// C++ Wrapper around alarm_callback_t.
class Alarm {
 public:
  template <typename Tag, typename F>
  Alarm Create(F&& callback);

  template <typename F>
  Alarm Create(F&& callback) {
    return Create<F>(std::forward<F>(callback));
  }

  Alarm() = default;
  Alarm(Alarm&& other) { *this = std::move(other); }
  Alarm& operator=(Alarm&& other) {
    if (this != &other) {
      state_ = std::exchange(other.state_, nullptr);
    }
    return *this;
  }

  ~Alarm();

  bool Cancel();

  void ScheduleAt(absolute_time_t time);

 private:
  struct State;
  template <typename Tag, typename F>
  struct Singleton;
  State* state_ = nullptr;
};

struct Alarm::State {
  alarm_pool_t* pool;
  alarm_id_t id = -1;
  alarm_callback_t callback = nullptr;

  bool Cancel() { return alarm_pool_cancel_alarm(pool, id); }

  void ScheduleAt(absolute_time_t time) {
    const bool fire_if_past = true;
    void* const user_data = nullptr;
    id = alarm_pool_add_alarm_at(pool, time, callback, user_data, fire_if_past);
  }
};

template <typename Tag, typename F>
struct Alarm::Singleton {
  inline static State state = {};
  static std::optional<F> callback = std::nullopt;
  static int64_t AlarmCallback(alarm_id_t id, void* user_data) {
    (*callback)();
    return 0;
  }
};

template <typename Tag, typename F>
Alarm Alarm::Create(F&& callback) {
  using SingletonType = Singleton<Tag, F>;
  SingletonType::callback.emplace(callback);
  State& state = SingletonType::state;
  state.pool = alarm_pool_get_default();
  state.callback = &SingletonType::AlarmCallback;
  Alarm alarm;
  alarm.state_ = &state;
  return alarm;
}

inline Alarm::~Alarm() {
  if (state_ != nullptr) {
    Cancel();
  }
}

inline bool Alarm::Cancel() { return state_->Cancel(); }

inline void Alarm::ScheduleAt(absolute_time_t time) {
  state_->ScheduleAt(time);
}
