#include "Clock.hpp"

auto Clock_domain::operator co_await() const noexcept -> Clock_domain::Awaiter
{
	return Awaiter{*this};
}
auto Clock_domain::notify_edge() const -> void
{
	if(_edge_awaiter.size() == 0) {
		return;
	}
	for(auto it : _edge_awaiter) {
		(*it)._cycle--;
	}
	auto end_it = std::partition(std::begin(_edge_awaiter), std::end(_edge_awaiter),
	                             [](const auto elt) { return (*elt).get_cycle() == 0; });

	if((_edge_awaiter[0])->get_cycle() == 0) {
                std::cout<<"Notifying Edge" << std::endl;
		auto resume_awaiter = std::span(std::begin(_edge_awaiter), end_it);
		for(const auto it : resume_awaiter) {
			it->coroutineHandle.resume();
		}
		_edge_awaiter.erase(std::begin(_edge_awaiter),
		                    std::begin(_edge_awaiter) + resume_awaiter.size());
	}
}

Clock_domain::Poll_timer::Poll_timer(double usec) noexcept
{
	_clock_fd = timerfd_create(CLOCK_REALTIME, 0);
	if(_clock_fd == -1) {
		std::terminate();
	}
	_timerValue.it_value.tv_sec = 0;
	_timerValue.it_value.tv_nsec = usec;
	_timerValue.it_interval.tv_sec = 0;
	_timerValue.it_interval.tv_nsec = usec;
	start_timer();
}

auto Clock_domain::Poll_timer::start_timer() const noexcept -> void
{
	/* start timer */
	if(timerfd_settime(_clock_fd, 0, &_timerValue, NULL) < 0) {
		std::cout << "could not start timer" << '\n';
		std::terminate();
	}
}

auto Clock_domain::Awaiter::await_ready() const noexcept -> bool { return false; }

auto Clock_domain::Awaiter::await_suspend(std::coroutine_handle<> coro) noexcept -> bool
{
	coroutineHandle = coro;
	_event._edge_awaiter.push_back(this);
	return true;
}
