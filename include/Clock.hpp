#ifndef __CLOCK_HPP__
#define __CLOCK_HPP__
#include "include_std.hpp"
#include <chrono>
#include <coroutine>
#include <ctime>
#include <iostream>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
class Clock_domain {
  public:
	Clock_domain(double sec) : _clock_domain(sec) { _clock_domain.start_timer(); }
	Clock_domain(const Clock_domain &) = delete;
	Clock_domain(Clock_domain &&) = delete;
	auto operator=(const Clock_domain &) -> Clock_domain & = delete;
	auto operator=(Clock_domain &&) -> Clock_domain & = delete;

	// the resumable class
	class Awaiter;
	auto operator co_await() const noexcept -> Awaiter;
	// use to resume all the registered coroutine
	auto notify_edge() const -> void;
	// use to controlle the underlying timer with epoll see Scheduler below
	auto start_timer() noexcept -> void { _clock_domain.start_timer(); }
	auto timer_fd() const noexcept -> int { return _clock_domain._clock_fd; }

  private:
	struct Poll_timer {
		int _clock_fd;
		struct itimerspec _timerValue;
		Poll_timer(double usec) noexcept;
		Poll_timer() noexcept : Poll_timer(1){};
		~Poll_timer() noexcept { close(_clock_fd); };
		auto start_timer() const noexcept -> void;
	};
	// to store pending awaiter
	mutable std::vector<Awaiter *> _edge_awaiter;
	// to store awaiter to be resumed on a notify_edge call
	// i.e. a std::movee of _edge_awaiter to avoid infinite loop
	// that are induce by loop on co_await when resuming.
	mutable std::vector<Awaiter *> _resume_awaiter;
	// the timer
	mutable Poll_timer _clock_domain;
};
class Clock_domain::Awaiter {
  public:
	Awaiter(const Clock_domain &clock, int cycle, std::optional<int> promise)
	    : _event(clock), _cycle(cycle), _promise(promise){};
	Awaiter(const Clock_domain &clock, int cycle) : _event(clock), _cycle(cycle){};
	Awaiter(const Clock_domain &clock) : Awaiter(clock, 1, {}){};
	// we always suspend since we stop on a cycle so it returns false
	auto await_ready() const noexcept -> bool;
	// here we register awaiter in Clock::_edge_awaiter
	auto await_suspend(std::coroutine_handle<> coro) noexcept -> bool;
	// nothing to be done on resume
	auto await_resume() const noexcept -> std::optional<int> { return _promise; };

	auto get_cycle() const noexcept -> int { return _cycle; }

  private:
	friend class Clock_domain;
	// the event we registered to
	const Clock_domain &_event;
	int _cycle;
	// TODO check the promise type ?
	std::optional<int> _promise;
	// the coroutine frame to be called in notify
	std::coroutine_handle<> coroutineHandle;
};

class Scheduler {
	// This class is use to manage the different clock domain One drawback of such a
	// scheme is that we have one timer by domain On small component it is fine but maybe
	// we can manage this better
  public:
	~Scheduler() { close(_efd); }
	Scheduler() = delete;
	explicit Scheduler(std::initializer_list<Clock_domain *> timers) noexcept
	    : _timer{timers}
	{
		struct epoll_event ev;
		_efd = epoll_create1(0);
		if(_efd < 0) {
			std::terminate();
		}
		for(size_t idx = 0; auto it : _timer) {
			ev.events = EPOLLIN;
			ev.data.fd = it->timer_fd();
			ev.data.ptr = it;
			auto error_code = epoll_ctl(_efd, EPOLL_CTL_ADD, it->timer_fd(), &ev);
			if(error_code < 0) {
				std::terminate();
			}
			idx += 1;
		}
	}
	auto operator()() noexcept -> void
	{

		struct epoll_event events;
		int nr_events = epoll_wait(_efd, &events, 1, -1);
		int tmp = errno;
		if(not(nr_events < 0) and not(tmp < 0)) {
			auto ptimer = static_cast<Clock_domain *>(events.data.ptr);
			ptimer->start_timer();
			ptimer->notify_edge();
		}
	}

  private:
	// the different clock domain handle by our scheduler
	std::vector<Clock_domain *> _timer;
	int _efd;
};
// empty task needed by coroutine framework
// Indeeed when resuming to the caller with co_await we need to return
// a type with a promise_type here it is void type.
struct Void_task {
	// needed for co_await promise type
	struct promise_type {
		Void_task get_return_object() { return {}; }
		auto initial_suspend() { return std::suspend_never{}; }
		auto final_suspend() { return std::suspend_never{}; }
		void return_void() {}
		void unhandled_exception() {}
	};
};

// TODO Constexpr ?
inline auto make_awaiter(const Clock_domain &clock, int cycle, int promise)
{
	return Clock_domain::Awaiter{clock, cycle, promise};
}
inline auto make_awaiter(const Clock_domain &clock, int cycle)
{
	return Clock_domain::Awaiter{clock, cycle};
}
// return µs result
constexpr long double operator"" _Mhz(long double frequency)
{
	return (frequency == 0) ? 0 : (1e3 / frequency);
}
// return µs result
constexpr long double operator"" _Mhz(unsigned long long int frequency)
{
	return (frequency == 0) ? 0 : (1e3 / frequency);
}

#endif