#pragma once
#include <cstdint>
#include <vector>
#include <algorithm>
#include <stdexcept>

template<typename T>
class Buffer 
{
public:
	Buffer(int w, int h)
		:mWidth(w), mHeight(h)
	{
		mData.resize(w * h);
	}

	//读和写 (x, y) 处的元素（y 向下，左上角原点，匹配 SDL）
	T& operator()(int x, int y) {
		if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) {
			throw std::out_of_range("Buffer index out of range");
		}
		return mData[y * mWidth + x];
	}

	// 常量版本：只能读
	const T& operator()(int x, int y) const {
		if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) {
			throw std::out_of_range("Buffer index out of range");
		}
		return mData[y * mWidth + x];
	}


	//整体清成某个值
	void clear(const T& value)
	{
		std::fill(mData.begin(), mData.end(), value);
	}

	//获取宽度和高度
	int Width() const { return mWidth; }
	int Height() const { return mHeight; }

	//获取底层数据指针
	T* data() { return mData.data(); }

private:
	int mWidth = 0;
	int mHeight = 0;
	std::vector<T> mData;

};

//两种常用实例
using ColorBuffer = Buffer<uint32_t>;
using DepthBuffer = Buffer<float>;
