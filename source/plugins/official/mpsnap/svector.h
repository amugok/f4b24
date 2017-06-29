template <class T>
class  SimpleVector
{
public:

	typedef T* iterator;

	SimpleVector();
	SimpleVector(unsigned int size);
	SimpleVector(unsigned int size, const T & initial);
	SimpleVector(const SimpleVector<T>& v);
	~SimpleVector();

	unsigned int capacity() const;
	unsigned int size() const;
	bool empty() const;
	iterator begin();
	iterator end();
	T& front();
	T& back();
	void push_back(const T& value);
	void pop_back();

	void reserve(unsigned int capacity);
	void resize(unsigned int size);

	T & operator[](unsigned int index);
	SimpleVector<T> & operator = (const SimpleVector<T> &);
	void clear();
	void erase(iterator p);
	void insert(iterator p, T & s);
private:
	unsigned int _size;
	unsigned int _capacity;
	unsigned int Log;
	T* buffer;
};

template<class T>
SimpleVector<T>::SimpleVector()
{
	_capacity = 0;
	_size = 0;
	buffer = 0;
	Log = 0;
}

template<class T>
SimpleVector<T>::SimpleVector(const SimpleVector<T> & v)
{
	_size = v._size;
	Log = v.Log;
	_capacity = v._capacity;
	buffer = (T *)LocalAlloc(LPTR, _capacity * sizeof(T));
	for (unsigned int i = 0; i < _size; i++)
		buffer[i] = v.buffer[i];
}

template<class T>
SimpleVector<T>::SimpleVector(unsigned int size)
{
	_size = size;
	Log = ceil(log((double) size) / log(2.0));
	_capacity = 1 << Log;
	buffer = (T *)LocalAlloc(LPTR, _capacity * sizeof(T));
}

template <class T>
bool SimpleVector<T>:: empty() const
{
	return _size == 0;
}

template<class T>
SimpleVector<T>::SimpleVector(unsigned int size, const T& initial)
{
	_size = size;
	Log = ceil(log((double) size) / log(2.0));
	_capacity = 1 << Log;
	buffer = (T *)LocalAlloc(LPTR, _capacity * sizeof(T));
	for (unsigned int i = 0; i < size; i++)
		buffer[i] = initial;
}

template<class T>
SimpleVector<T>& SimpleVector<T>::operator = (const SimpleVector<T> & v)
{
	if (!buffer)
	{
		LocalFree(buffer);
		buffer = 0;
	}
	_size = v._size;
	Log = v.Log;
	_capacity = v._capacity;
	buffer = (T *)LocalAlloc(LPTR, _capacity * sizeof(T));
	for (unsigned int i = 0; i < _size; i++)
		buffer[i] = v.buffer[i];
	return *this;
}

template<class T>
typename SimpleVector<T>::iterator SimpleVector<T>::begin()
{
	return buffer;
}

template<class T>
typename SimpleVector<T>::iterator SimpleVector<T>::end()
{
	return buffer + size();
}

template<class T>
T& SimpleVector<T>::front()
{
	return buffer[0];
}

template<class T>
T& SimpleVector<T>::back()
{
	return buffer[_size - 1];
}

template<class T>
void SimpleVector<T>::push_back(const T & v)
{
	if (_size >= _capacity)
	{
		reserve(1 << Log);
		Log++;
	}
	buffer [_size++] = v;
}

template<class T>
void SimpleVector<T>::pop_back()
{
	_size--;
}

template<class T>
void SimpleVector<T>::reserve(unsigned int capacity)
{
	T * newBuffer = (T *)LocalAlloc(LPTR, capacity * sizeof(T));

	for (unsigned int i = 0; i < _size; i++)
		newBuffer[i] = buffer[i];

	_capacity = capacity;
	if (!buffer)
	{
		LocalFree(buffer);
		buffer = 0;
	}
	buffer = newBuffer;
}

template<class T>
unsigned int SimpleVector<T>::size() const
{
	return _size;
}

template<class T>
void SimpleVector<T>::resize(unsigned int size)
{
	Log = ceil(log((double) size) / log(2.0));
	reserve(1 << Log);
	_size = size;
}

template<class T>
T& SimpleVector<T>::operator[](unsigned int index)
{
	return buffer[index];
}

template<class T>
unsigned int SimpleVector<T>::capacity()const
{
	return _capacity;
}

template<class T>
SimpleVector<T>::~SimpleVector()
{
	if (!buffer)
	{
		LocalFree(buffer);
		buffer = 0;
	}
}

template <class T>
void SimpleVector<T>::clear()
{
	if (!buffer)
	{
		LocalFree(buffer);
		buffer = 0;
	}
	_capacity = 0;
	_size = 0;
	Log = 0;
}

template <class T>
void SimpleVector<T>::erase(iterator p)
{
	for (unsigned int i = p - buffer + 1; i < _size; i++)
	{
		buffer[i - 1] = buffer[i];
	}
	--_size;
}

template <class T>
void SimpleVector<T>::insert(iterator p, T & s)
{
	int l = (p > buffer + _size) ? _size : (p - buffer);
	if (_size >= _capacity)
	{
		reserve(1 << Log);
		Log++;
	}
	for (unsigned int i = _size; l < i; --i)
	{
		buffer[i] = buffer[i - 1];
	}
	buffer[l] = s;
	++_size;
}
