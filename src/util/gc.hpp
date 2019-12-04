#pragma once

#include <unordered_set>
#include <unordered_map>
#include <vector>

#ifdef DEBUG_GC
#define IF_DEBUG_GC(x) x
#else
#define IF_DEBUG_GC(x)
#endif

namespace GC {
	class GCObject;
	
	void pin(GCObject* obj);
	void unpin(GCObject* obj);	
	
	void collect();
	void step();
	
	class GCObject {
	public:
		GCObject();
		GCObject(GCObject const&) = delete;
		virtual ~GCObject();
		
		void mark();
		void reset();
		bool isMarked();
		
	private:
		bool _marked;
		
		virtual void markChildren();
	};
	
	template<typename T>
	class Root {
	public:
		Root(T* obj);
		~Root();
		
		T* get();
		
		T& operator*();
		T* operator->();
		
	private:
		T* obj;
	};
}

#include "gc.tpp"
