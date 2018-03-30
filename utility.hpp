#ifndef SBASE__UTILITY_HPP_
#define SBASE__UTILITY_HPP_


class NonCopy {
	public:
		NonCopy() = default;
	private:
		NonCopy(const NonCopy &) = delete;
		NonCopy& operator=(const NonCopy &) = delete;
};

#endif /* SBASE__UTILITY_HPP_ */