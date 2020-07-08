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
	if(_resume_awaiter.size() < _edge_awaiter.size()) {
		_resume_awaiter.reserve(_edge_awaiter.size());
	}
	for(auto it : _edge_awaiter) {
		(*it)._cycle--;
	}
	auto helper = [](const auto elt) { return (*elt).get_cycle() != 0; };
	auto found = std::find_if(std::begin(_edge_awaiter), std::end(_edge_awaiter),
	                          [](const auto elt) { return (*elt).get_cycle() == 0; });
	if(not(found == std::end(_edge_awaiter))) {
		auto end_it =
		    std::partition(std::begin(_edge_awaiter), std::end(_edge_awaiter), helper);
		_resume_awaiter.insert(_resume_awaiter.end(), std::make_move_iterator(end_it),
		                       std::make_move_iterator(std::end(_edge_awaiter)));
		_edge_awaiter.erase(end_it, std::end(_edge_awaiter));
		for(const auto it : _resume_awaiter) {
			it->coroutineHandle.resume();
			_resume_awaiter.pop_back();
		}
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
