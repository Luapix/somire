#pragma once

namespace GC {
	template<typename T>
	Root<T>::Root() : obj(nullptr) {}
	
	template<typename T>
	Root<T>::Root(T* obj) : obj(obj) {
		pin(obj);
	}
	
	template<typename T>
	Root<T>::Root(Root<T>&& other) : obj(nullptr) {
		swap(other);
	}
	
	template<typename T>
	Root<T>::~Root() {
		if(obj) unpin(obj);
	}
	
	template<typename T>
	T* Root<T>::get() { return obj; }
	template<typename T>
	T* Root<T>::release() {
		T* temp = obj;
		if(obj) {
			unpin(obj);
			obj = nullptr;
		}
		return temp;
	}
	template<typename T>
	void Root<T>::reset(T* obj2) {
		if(obj) unpin(obj);
		obj = obj2;
		if(obj) pin(obj);
	}
	
	template<typename T>
	T& Root<T>::operator*() { return *obj; }
	template<typename T>
	T* Root<T>::operator->() { return obj; }
	
	template<typename T>
	Root<T>& Root<T>::operator=(Root<T>&& other) {
		if(obj) unpin(obj);
		obj = other.obj;
		other.obj = nullptr;
		return *this;
	}
	
	template<typename T>
	void Root<T>::swap(Root<T>& other) {
		T* temp = other.obj;
		other.obj = obj;
		obj = temp;
	}
}
