#pragma once

namespace dh
{
namespace util
{

class NonCopyable
{
public:
	NonCopyable() {}
	~NonCopyable() {}

private:
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator = (const NonCopyable&) = delete;
};

}
}
