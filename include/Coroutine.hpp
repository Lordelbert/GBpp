#ifndef __COROUTINE_HPP__
#define __COROUTINE_HPP__
#include "include_std.hpp"
struct Dummy_coro {
	struct promise_type {
		Dummy_coro get_return_object() { return {}; }
		auto initial_suspend() { return std::suspend_never{}; }
		auto final_suspend() { return std::suspend_never{}; }
		void return_void() {}
		void unhandled_exception() {}
	};
};

// https://www.youtube.com/watch?v=8C8NnE1Dg4A&t=45m50s (Gor Nishanov, CppCon 2016)

template <class T> struct task {
	struct promise_type {
		// the coroutine which have call co_await
		std::variant<std::monostate, T, std::exception_ptr> result_;
		std::coroutine_handle<void> waiter_;

		task get_return_object() { return task(this); }
		// triggered only if co_await is called
		// see co_await for the call to resume
		auto initial_suspend() { return std::suspend_always{}; }
		auto final_suspend()
		{
			struct Awaiter {
				promise_type *me_;
				bool await_ready() { return false; }
				void await_suspend([[maybe_unused]] std::coroutine_handle<void> caller)
				{
					me_->waiter_.resume();
				}
				void await_resume() {}
			};
			return Awaiter{this};
		}
		template <class U> void return_value(U &&u)
		{
			result_.template emplace<1>(static_cast<U &&>(u));
		}
		void unhandled_exception()
		{
			result_.template emplace<2>(std::current_exception());
		}
	};

	// force calling await suspend and then since suspend return void return to caller
	bool await_ready() { return false; }
	// Return value
	// so that auto result = co_await task
	void await_suspend(std::coroutine_handle<void> caller)
	{
		coro_.promise().waiter_ = caller;
		coro_.resume();
	}
	T await_resume()
	{
		if(coro_.promise().result_.index() == 2) {
			std::rethrow_exception(std::get<2>(coro_.promise().result_));
		}
		return std::get<1>(coro_.promise().result_);
	}

	~task() { coro_.destroy(); }

  private:
	using handle_t = std::coroutine_handle<promise_type>;
	task(promise_type *p) : coro_(handle_t::from_promise(*p)) {}
	// the current coroutine
	handle_t coro_;
};

template <> struct task<void> {
	struct promise_type {
		// the coroutine which have call co_await
		std::exception_ptr result_;
		std::coroutine_handle<void> waiter_;

		task get_return_object() { return task(this); }
		auto initial_suspend() { return std::suspend_always{}; }
		auto final_suspend()
		{
			struct Awaiter {
				promise_type *me_;
				bool await_ready() { return false; }
				void await_suspend([[maybe_unused]] std::coroutine_handle<void> caller)
				{
					me_->waiter_.resume();
				}
				void await_resume() {}
			};
			return Awaiter{this};
		}
		void return_void() {}
		void unhandled_exception() { result_ = std::current_exception(); }
	};
	// force calling await suspend and then since suspend return void return to caller
	bool await_ready() { return false; }
	// Return value
	// so that auto result = co_await task
	void await_suspend(std::coroutine_handle<void> caller)
	{
		coro_.promise().waiter_ = caller;
		coro_.resume();
	}
	void await_resume()
	{
		if(coro_.promise().result_ != nullptr) {
			std::rethrow_exception(coro_.promise().result_);
		}
		return;
	}

	~task() { coro_.destroy(); }

  private:
	using handle_t = std::coroutine_handle<promise_type>;
	task(promise_type *p) : coro_(handle_t::from_promise(*p)) {}
	// the current coroutine ?
	handle_t coro_;
};

#endif
