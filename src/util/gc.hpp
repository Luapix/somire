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
		
	protected:
		virtual void markChildren();
		
	private:
		bool _marked;
	};
	
	template<typename T>
	class Root {
	public:
		Root();
		Root(T* obj);
		Root(Root<T>&&);
		~Root();
		
		T* get();
		T* release();
		void reset(T* obj2);
		
		T& operator*();
		T* operator->();
		
		Root<T>& operator=(Root<T>&&);
		
		void swap(Root<T>& other);
		
	private:
		T* obj;
	};
	
	template<typename T>
	class Vector : public GCObject {
	public:
		std::vector<T*> vec;
		
	protected:
		void markChildren() override;
	};
	
	template<typename T>
	using RootVector = Root<Vector<T>>;
	
	template<typename T>
	GC::RootVector<T> makeRootVector();
}

#include "gc.tpp"
