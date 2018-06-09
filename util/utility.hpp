#ifndef SBASE_UTIL_HPP_
#define SBASE_UTIL_HPP_


class NoCopy {
	public:
		NoCopy() = default;
	private:
		NoCopy(const NoCopy &) = delete;
		NoCopy& operator=(const NoCopy &) = delete;
};

#endif /* SBASE_UTIL_HPP_ */