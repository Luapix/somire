#pragma once

namespace GC {
	template<typename T>
	Root<T>::Root(T* obj) : obj(obj) {
		pin(obj);
	}
	
	template<typename T>
	Root<T>::~Root() {
		unpin(obj);
	}
	
	template<typename T>
	T* Root<T>::get() { return obj; }
	
	template<typename T>
	T& Root<T>::operator*() { return *obj; }
	template<typename T>
	T* Root<T>::operator->() { return obj; }
}
