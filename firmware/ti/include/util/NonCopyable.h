#pragma once

namespace util
{

	class NonCopyable
	{
	protected:
		NonCopyable() {}
		~NonCopyable() {}

	private:
		NonCopyable(const NonCopyable&) {}
		NonCopyable& operator= (const NonCopyable&) {return *this;}
	};

}
