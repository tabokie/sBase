#ifndef SBASE_UTIL_HPP_
#define SBASE_UTIL_HPP_


class NonCopy {
	public:
		NonCopy() = default;
	private:
		NonCopy(const NonCopy &) = delete;
		NonCopy& operator=(const NonCopy &) = delete;
};

#endif /* SBASE_UTIL_HPP_ */