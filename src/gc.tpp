#pragma once

namespace GC {
	template<typename T>
	GCRoot<T>::GCRoot(T* obj) : obj(obj) {
		pin(obj);
	}
	
	template<typename T>
	GCRoot<T>::~GCRoot() {
		unpin(obj);
	}
	
	template<typename T>
	T* GCRoot<T>::get() { return obj; }
	
	template<typename T>
	T& GCRoot<T>::operator*() { return *obj; }
	template<typename T>
	T* GCRoot<T>::operator->() { return obj; }
}
