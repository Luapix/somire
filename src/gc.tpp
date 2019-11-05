#pragma once

namespace GC {
	template<typename T>
	void GCVector<T>::markChildren() {
		for(T* obj : vec) {
			obj->mark();
		}
	}
}
