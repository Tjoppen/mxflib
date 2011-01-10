/*! \file	smartptr.h
 *	\brief	Smart Pointer class
 *
 *			This file contains the SmartPtr class (and helpers) originally
 *			written by Sandu Turcan and submitted to www.codeproject.com
 *
 *	\version $Id: smartptr.h,v 1.8 2011/01/10 10:42:09 matt-beard Exp $
 *
 */
/*
 *	Important Note: 
 *	This code originally comes from www.codeproject.com
 *	and was written by Sandu Turcan (idlsoft@hotmail.com)
 *	The code has been modified, with permission, and in this
 *	version is covered by the terms of the following licence:
 *
 *	Copyright (c) 2003, Matt Beard
 *
 *	This software is provided 'as-is', without any express or implied warranty.
 *	In no event will the authors be held liable for any damages arising from
 *	the use of this software.
 *
 *	Permission is granted to anyone to use this software for any purpose,
 *	including commercial applications, and to alter it and redistribute it
 *	freely, subject to the following restrictions:
 *
 *	  1. The origin of this software must not be misrepresented; you must
 *	     not claim that you wrote the original software. If you use this
 *	     software in a product, an acknowledgment in the product
 *	     documentation would be appreciated but is not required.
 *	
 *	  2. Altered source versions must be plainly marked as such, and must
 *	     not be misrepresented as being the original software.
 *	
 *	  3. This notice may not be removed or altered from any source
 *	     distribution.
 */

#ifndef MXFLIB__SMARTPTR_H
#define MXFLIB__SMARTPTR_H

// Pointer debug enable and disable
//#define PTRDEBUG( x ) x
#define PTRDEBUG( x )

// Pointer checking enable and disable (takes a lot of CPU time)
// If this is enabled the application code must instantiate and check PtrCheckList
//#define PTRCHECK( x ) x
#define PTRCHECK( x )

#ifndef NO_SP_MUTEX
#ifdef _WIN32
#include <assert.h>
#else
#include <pthread.h>
#endif
#endif //NO_SP_MUTEX

// Ensure we know NULL
#include <stdlib.h>

namespace mxflib 
{
	// Forward declaration of SmartPtr to allow it to be befreinded
	template <class T> class SmartPtr;

	// Forward declare ParentPtr to allow it to be used in RefCount<>
	template <class T> class ParentPtr;

	//! An interface for reference counting
	/*! Classes may implement it themselves, or SmartPtr will
	*  provide its internal implementation of IRefCount
	*/
	template <class T> class IRefCount 
	{
		// Allow SmartPtr access to our internals
		friend class SmartPtr<T>;

		// Allow ParentPtr access to our internals
		friend class ParentPtr<T>;

		// Allow any SmartSubPtr access
		template <class Base, class Derived> friend class SmartSubPtr;

	protected:
		// A number of pure virtual functions that the reference counter needs

		virtual void __IncRefCount() =0;				//!< Increment the number of references
		virtual void __DecRefCount() =0;				//!< Decrement the number of references, if none left delete the object
		virtual T * GetPtr() =0;						//!< Get a pointer to the object
		virtual IRefCount<T> * GetRefC() =0;			//!< Get a pointer to the reference counter

		virtual void AddRefC(ParentPtr<T> &Ptr) =0;		//!< Add a parent pointer to this object
		virtual void DeleteRef(ParentPtr<T> &Ptr) =0;	//!< Delete a parent pointer to this object
		virtual void ClearParents(void) =0;				//!< Clear all parent pointers

		virtual ~IRefCount() { };
	};

	// Definitions for running memory leak tests
	typedef std::pair<void*,std::string> PtrCheckListItemType;
	typedef std::list<PtrCheckListItemType> PtrCheckListType;
	PTRCHECK( extern PtrCheckListType PtrCheckList; )
}


namespace mxflib 
{

	//! Standard implementation of IRefCount
	/*! Classes derived from RefCount will contain their own reference
	*  counting implementation and allow more efficient memory use.
	*	
	*	\par Usage:
	*	To derive a class from RefCount use:
	*	<br><tt>class MyClass : public RefCount<MyClass> {...};</tt>
	*
	*	\note 
	*	It is not necessary to derive a class from RefCount to
	*  use it with SmartPtr, but it makes more efficient use of memory
	*  if you do.
	*/
	template <class T> class RefCount : public IRefCount<T>
	{

	protected:
		int __m_counter;									//!< The actual reference count

		typedef ParentPtr<T> LocalParent;					//!< Parent pointer to this type
		typedef std::list<LocalParent*> LocalParentList;	//!< List of pointers to parent pointers
		LocalParentList *ParentPointers;					//!< List of parent pointers to this object

#ifndef NO_SP_MUTEX
#ifdef _WIN32
		CRITICAL_SECTION mutex; 
#else
		pthread_mutex_t mutex;
#endif
#endif //NO_SP_MUTEX

	protected:
		//! Increment the number of references
		virtual void __IncRefCount()
		{
#ifndef NO_SP_MUTEX
#ifdef _WIN32
			EnterCriticalSection(& mutex);
#else
			pthread_mutex_lock( & mutex);
#endif
#endif //NO_SP_MUTEX
			__m_counter++;

			PTRDEBUG( debug("%p Increment count -> %d\n", this, __m_counter); )
#ifndef NO_SP_MUTEX
#ifdef _WIN32
				LeaveCriticalSection(&mutex);
#else
				pthread_mutex_unlock( &mutex);
#endif
#endif //NO_SP_MUTEX
		}

		//! Decrement the number of references, if none left delete the object
		virtual void __DecRefCount()
		{
#ifndef NO_SP_MUTEX
#ifdef _WIN32
			EnterCriticalSection(& mutex);
#else
			pthread_mutex_lock( & mutex);
#endif
#endif //NO_SP_MUTEX

			__m_counter--;

			PTRDEBUG( debug("%p Decrement count -> %d\n", this, __m_counter); )

				if(__m_counter<=0)
				{
#ifndef NO_SP_MUTEX
#ifdef _WIN32
					LeaveCriticalSection(&mutex);
#else
					pthread_mutex_unlock( &mutex);
#endif
#endif //NO_SP_MUTEX
					__DestroyRef();
				}
				else
				{
#ifndef NO_SP_MUTEX
#ifdef _WIN32
					LeaveCriticalSection(&mutex);
#else
					pthread_mutex_unlock( &mutex);
#endif
#endif //NO_SP_MUTEX
				}
		}

		//! Get a pointer to the object
		virtual T * GetPtr()
		{
			return ((T *)this);
		}

		//! Get a pointer to the object
		virtual IRefCount<T>* GetRefC()
		{
			return this;
		}

		//! Get a const pointer to the object
		virtual const IRefCount<T>* GetRefC() const
		{
			return this;
		}

		//! Destroy the object
		/*! Called once all references have gone
		*/
		virtual void __DestroyRef() 
		{ 
			if(GetPtr()!=NULL)
			{
				// If we are "checking" locate and remove our entry
				PTRCHECK
					( 
					PtrCheckListType::iterator it = PtrCheckList.begin();
				while(it != PtrCheckList.end())
				{
					if((*it).first == (void*)this)
					{
						PtrCheckList.erase(it);
						break;
					}
					it++;
				}
				)

					delete GetPtr();
			}
		}

		//! Add a parent pointer to this object
		virtual void AddRefC(ParentPtr<T> &Ptr)
		{
#ifndef NO_SP_MUTEX
#ifdef _WIN32
			EnterCriticalSection(& mutex);
#else
			pthread_mutex_lock( & mutex);
#endif
#endif //NO_SP_MUTEX
			PTRDEBUG( debug("Adding ParentPtr(%p) to %p\n", &Ptr, this); )

				if(!ParentPointers) ParentPointers = new LocalParentList;
			ParentPointers->push_back(&Ptr);
#ifndef NO_SP_MUTEX
#ifdef _WIN32
			LeaveCriticalSection(&mutex);
#else
			pthread_mutex_unlock( &mutex);
#endif
#endif //NO_SP_MUTEX
		}

		//! Delete a parent pointer to this object
		virtual void DeleteRef(ParentPtr<T> &Ptr)
		{
#ifndef NO_SP_MUTEX
#ifdef _WIN32
			EnterCriticalSection(& mutex);
#else
			pthread_mutex_lock( & mutex);
#endif
#endif //NO_SP_MUTEX

			if(ParentPointers)
			{
				typename LocalParentList::iterator it = ParentPointers->begin();
				while(it != ParentPointers->end())
				{
					if((*it) == &Ptr)
					{
						PTRDEBUG( debug("Deleting ParentPtr(%p) from %p\n", &Ptr, this); )
							ParentPointers->erase(it);
#ifndef NO_SP_MUTEX
#ifdef _WIN32
						LeaveCriticalSection(&mutex);
#else
						pthread_mutex_unlock( &mutex);
#endif
#endif //NO_SP_MUTEX

						return;
					}
					it++;
				}
			}
			error("Tried to clear ParentPtr(%p) from %p but that ParentPtr does not exist\n", &Ptr, this);
#ifndef NO_SP_MUTEX
#ifdef _WIN32
			LeaveCriticalSection(&mutex);
#else
			pthread_mutex_unlock( &mutex);
#endif
#endif //NO_SP_MUTEX
		}

	protected:
		//! Constructor for the RefCount class
		RefCount()
		{
#ifndef NO_SP_MUTEX
#ifdef _WIN32
			InitializeCriticalSection(&mutex);
#else
			pthread_mutex_init(&mutex, NULL);
#endif
#endif //NO_SP_MUTEX			
			// If we are "checking" add entry to the list
			// We add to the start of the list as this gives the best chance of finding the
			// item quickly when it is deleted (most objects are first-in-last-out)
			PTRCHECK( PtrCheckList.push_front(PtrCheckListItemType((void*)this, PrintInfo())); )

				// No references yet!
				__m_counter = 0;

			// No parent pointers (yet) reference this item
			ParentPointers = NULL;

			PTRDEBUG( debug("%p Build new (zero) count\n", this); )
		}

		//! Copy Constructor for the RefCount class
		RefCount( RefCount &)
		{
#ifndef NO_SP_MUTEX
#ifdef _WIN32
			InitializeCriticalSection(&mutex);
#else
			pthread_mutex_init(&mutex, NULL);
#endif
#endif //NO_SP_MUTEX			
			// If we are "checking" add entry to the list
			// We add to the start of the list as this gives the best chance of finding the
			// item quickly when it is deleted (most objects are first-in-last-out)
			PTRCHECK( PtrCheckList.push_front(PtrCheckListItemType((void*)this, PrintInfo())); )

			// No references yet!
			__m_counter = 0;

			// No parent pointers (yet) reference this item
			ParentPointers = NULL;

			PTRDEBUG( debug("%p Copy Construct new (zero) count\n", this); )
		}


		//! Referenced object has been set using operator=() - we must deliberately do nothing to avoid copying the reference count
		RefCount & operator=(const RefCount & RHS)
		{
			// DRAGONS: We genuinely do nothing here as assigning a new value to the referenced object does not change the count of references
			return *this;
		}


		//! Clear all parent pointers when we are destroyed
		virtual ~RefCount() 
		{
			if(ParentPointers) ClearParents(); 
#ifndef NO_SP_MUTEX
#ifdef _WIN32
			DeleteCriticalSection(&mutex);
#else
			pthread_mutex_unlock( &mutex);
			pthread_mutex_destroy(&mutex);
#endif
#endif //NO_SP_MUTEX
		}



		//! Clear all parent pointers
		virtual void ClearParents(void);

		PTRCHECK
			(
	public:
		virtual std::string PrintInfo(void)
		{
			char buffer[1024];
			sprintf(&buffer[0], "Item size = %d :", sizeof(T));
			for(int i=0; i<min(sizeof(T), 64); i++) sprintf(&buffer[strlen(buffer)], " %02x", ((UInt8*)(this))[i]);
			return std::string(buffer);
		}

		void SetDebug(std::string str)
		{
			PtrCheckListType::iterator it = PtrCheckList.begin();
			while(it != PtrCheckList.end())
			{
				if((*it).first == (void*)this)
				{
					PTRDEBUG( debug("Setting text to (%s)\n", str.c_str()); )
						(*it).second = str;
					return;
				}
				it++;
			}
		}
		)
	};

	// Clear all parent pointers
	template <class T> void mxflib::RefCount<T>::ClearParents(void)
	{
		if(ParentPointers)
		{
			typename LocalParentList::iterator it = ParentPointers->begin();
			while(it != ParentPointers->end())
			{
				(*it)->ClearFromParent();
				it++;
			}

			delete ParentPointers;
		}
	}
}


namespace mxflib
{
	//! Smart pointer with reference counting and auto object deletion
	/*!	<b>Usage:</b>
	*
	*	1. In a program block
	*	<pre><tt>
	*	SmartPtr<MyClass> ptr1(new MyClass); // creates object 1
	*	SmartPtr<MyClass> ptr2(new MyClass); // creates object 2
	*
	*	ptr1 = ptr2;            // destroys object 1
	*	ptr2 = NULL;
	*
	*	ptr1 = new MyClass;		// creates object 3, destroys object 2
	*	ptr1->methodcall(...);
	*
	*	MyClass o1;
	*	// ptr1 = &amp;o1;  // DON'T ! only memory allocated by new operator should be used
	*
	*	CMyObject *o2 = new MyClass;
	*	ptr1 = o2;
	*	
	*	//ptr2 = o2;  // DON'T ! unless MyClass implements IRefCount
	*	              // try to use ptr1 = ptr2 instead, it's always safe;
	*	</tt></pre>
	*	2. In a function call
	*	<pre><tt>
	*	void func(MyClass *o) {...}
	*	...
	*	SmartPtr<MyClass> ptr(new MyClass);
	*	func(ptr);
	*	</tt></pre>
	*	3. As a return value
	*	<pre><tt>
	*	SmartPtr<MyClass> f()
	*	{
	*		SmartPtr<MyClass> ptr(new MyClass);
	*		return ptr;
	*	}
	*	</tt></pre>
	*	4. Accessing members
	*	<pre><tt>
	*	SmartPtr<MyClass> ptr(new MyClass);
	*	ptr->ClassMember = 0;
	*	</tt></pre>
	*/
	template <class T> class SmartPtr 
	{
	protected:

		IRefCount<T> *__m_refcount;		//!< Pointer to the reference counted object

		//! Name to use when debugging this pointer
		PTRDEBUG( std::string DebugName; )

			//! Default IRefCount implementation used by SmartPtr
			/*! SmartPrt will automatically choose between its internal
			*	and the class' own implementation of IRefCount
			*/
		class __RefCounter : public RefCount<T>
		{
		private:
			T *__m_ptr;					//!< Pointer to the reference counted object
		protected:
			//! Get a pointer to the object
			virtual T * GetPtr() const { return __m_ptr; }

			//! Destroy the object
			/*! Called once all references have gone
			*/
			virtual void __DestroyRef() { 	delete this; }

		public:
			//! Construct a reference counter for an object
			__RefCounter(T *ptr)
			{
				__m_ptr = ptr;
			}

			//! Destroy the counter and its object
			virtual ~__RefCounter()
			{
				RefCount<T>::__DestroyRef();
			}
		};

		//! Assign a 'dumb' object to this smart pointer
		/*!	This method is called if T does not implement IRefCount.
		*	A default counter is instanciated and used for all referencing
		*/
		/*	DRAGONS: Removed to allow kludge to fix gcc 3.3.x bug
		void __Assign(void *ptr)
		{
		if(ptr==NULL)
		__Assign((IRefCount<T> *)NULL);
		else
		{
		__Assign(new __RefCounter(static_cast<T *>(ptr)));
		}
		}
		*/

		/*	DRAGONS:  KLUDGE to fix gcc 3.3.x bug - remove generic assign and force all smart pointer targets to be derived from IRefCount<> */
		/*	DRAGONS:  If this klidge is undone ParentPtr will need updating to ensure it works with non-RefCount versions */

		//!	Assign a 'smart' object to this smart pointer
		/*!	This method is picked over __Assign(void *ptr)
		*	if T implements IRefCount.
		*	This allows some memory usage optimization
		*	\note
		*	Assigning NULL will detatch this pointer from the
		*	current object
		*/
		virtual void __Assign(IRefCount<T> *refcount)
		{
			PTRDEBUG( if(DebugName.size()) debug("%s changing from %p to %p\n", DebugName.c_str(), __m_refcount, refcount); )

				// Attach us to the new object first
				// This is important in case we are assigned to
				// the same thing we are already attatched to,
				// in which case detatching first could cause
				// our object to be deleted!!

				if(refcount!=NULL) 
				{
					refcount->__IncRefCount();
				}

				// Record what we were attached to
				IRefCount<T> *oldref = __m_refcount;

				// Make the new attachment
				__m_refcount = refcount;

				// Break the old attachment
				if(oldref!=NULL) 
					oldref->__DecRefCount();
		}

	public:
		//! Construct a smart pointer that points to nothing
		SmartPtr()
		{
			__m_refcount = NULL;
		}

		//!	Construct a smart pointer to an object
		SmartPtr(IRefCount<T> * ptr)
		{
			__m_refcount = NULL;
			__Assign(ptr);
		}

		//!	Construct a smart pointer that takes its target from another smart pointer
		SmartPtr(const SmartPtr<T> &sp)
		{
			__m_refcount = NULL;
			__Assign(sp.__m_refcount);
		}

		//!	Construct a smart pointer from a parent pointer
		SmartPtr(const ParentPtr<T> &ptr)
		{
			this->__m_refcount = NULL;
			__Assign(ptr.GetRefC());
		}

		//! Detatch this pointer from the object before destruction
		virtual ~SmartPtr()
		{
			__Assign((IRefCount<T> *)NULL);
		}

		//! Get the contained pointer
		T *GetPtr() const
		{
			if(__m_refcount==NULL) return NULL;
			return __m_refcount->GetPtr();
		}

		//! Get the contained object's refcount
		IRefCount<T> *GetRefC() const
		{
			if(__m_refcount==NULL) return NULL;
			return __m_refcount->GetRefC();
		}

		//! Assign another smart pointer
		SmartPtr & operator = (const SmartPtr<T> &sp) {__Assign(sp.__m_refcount); return *this;}

		//! Assign pointer or NULL
		SmartPtr & operator = (IRefCount<T> * ptr) {__Assign(ptr); return *this;}

		//! Give access to members of T
		T * operator ->()
		{
			mxflib_assert(GetPtr()!=NULL);
			return GetPtr();
		}

		//! Give const access to members of T
		const T * operator ->() const
		{
			mxflib_assert(GetPtr()!=NULL);
			return (const T*)GetPtr();
		}

		//! Conversion to T* (for function calls)
		operator T* () const
		{
			return GetPtr();
		}

		//! Test for NULL
		bool operator !()
		{
			return GetPtr()==NULL;
		}

		//! Test for equality (ie. do both pointers point to the same object)
		bool operator ==(const SmartPtr &sp)
		{
			return GetPtr()==sp.GetPtr();
		}

		//! Test for inequality (ie. do pointers point to different objects)
		bool operator !=(const SmartPtr &sp)
		{
			return GetPtr()!=sp.GetPtr();
		}

		//! Comparison function to allow sorting by indexed value
		bool operator<(SmartPtr &Other) { return this.operator<(*Other->GetPtr()); }

		//! Get a cast version of the pointer
		/*! This is used via the SmartPtr_Cast() Macro to allow MSVC 6 to work!!
		*	The reason for this is that MSVC 6 name mangling is only based on the function arguments so
		*  it cannot cope when two functions differ in the template type, but not the argument list!!
		*  The solution is a dummy argument that gets filled in by the macro (to avoid messy code!)
		*/
		template <class U> U* Cast(U*) { return dynamic_cast<U*>(GetPtr()); } 

		//! Set the debug name
		PTRDEBUG( void SetDebugName(std::string Name) { DebugName = Name; } )
	};
}


namespace mxflib
{
	//! Parent pointer class - used to allow an object referenced by another object to make a returrn reference without forming a loop
	/*! If ObjectA has a smart pointer to ObjectB it shares ownership of it (and so ObjectA is a parent of ObjectB).
	*  A child may not hold a smart pointer to a parent (or grand-parent etc.) otherwise a loop will be formed and
	*  these objects will never be deleted.  Child objects may reference parents using ParentPtr.
	*/
	// DRAGONS: Strict C++ compilers (such as GCC 3.4.x) require members of superclass templates to be referenced by this-> to remove possilbe ambiguities
	template<class T> class ParentPtr : public SmartPtr<T>
	{
	protected:
		//!	Assign a 'smart' object to this pointer
		/*! Note that no reference counting is performed with this version.
		*  This prevents circular references keeping objects for ever.
		*/
		virtual void __Assign(IRefCount<T> *refcount)
		{
			PTRDEBUG( debug("Assigning parent pointer at %p to %p\n", this, refcount); )

				// Remove us from the old parent's list of parent pointers
				if(this->__m_refcount) this->__m_refcount->DeleteRef(*this);

			// Make the new attachment
			this->__m_refcount = refcount;

			// Add us to the new parent's list of parent pointers (so we will be cleared if it is deleted)
			if(refcount) refcount->AddRefC(*this);
		}

	public:
		//! Construct a parent pointer that points to nothing
		ParentPtr()
		{
			this->__m_refcount = NULL;
		}

		//!	Construct a parent pointer from a smart pointer
		ParentPtr(SmartPtr<T> ptr)
		{
			this->__m_refcount = NULL;
			__Assign(ptr.GetRefC());
		}

		//!	Construct a parent pointer to an object
		ParentPtr(IRefCount<T> * ptr)
		{
			this->__m_refcount = NULL;
			__Assign(ptr);
		}

		//! Copy construct
		ParentPtr(const ParentPtr &rhs)
		{
			this->__m_refcount = NULL;
			__Assign(rhs.GetRefC());
		}

		//! Destructor clears the pointer without decrementing the count
		~ParentPtr()
		{
			// Remove us from the old parent's list of parent pointers
			if(this->__m_refcount) (this->__m_refcount)->DeleteRef(*this);

			this->__m_refcount = NULL;
		}

		//! Set value from a smart pointer
		ParentPtr & operator=(const SmartPtr<T> &sp) { __Assign(sp.GetRefC()); return *this;}

		//! Set value from a parent pointer
		ParentPtr & operator=(const ParentPtr<T> &sp) { __Assign(sp.__m_refcount); return *this;}

		//! Set value from a pointer
		ParentPtr & operator=(T *Ptr) { __Assign((IRefCount<T>*)Ptr); return *this; }

		//! Clear the recorded value of this pointer
		void Clear(void) 
		{
			// Remove us from the old parent's list of parent pointers
			if(this->__m_refcount) this->__m_refcount->DeleteRef(*this);

			this->__m_refcount = NULL; 
		}

		//! Clear the recorded value of this pointer
		/*! This call <b>does not</b> remove us from the parent's list of parent pointers (called by the parent) */
		void ClearFromParent(void) { this->__m_refcount = NULL; };
	};

	template<class Base, class Derived> class SmartSubPtr : public SmartPtr<Base>
	{
	public:
		//! Construct a sub pointer that points to nothing
		SmartSubPtr()
		{
			this->__m_refcount = NULL;
		}

		//!	Construct a sub pointer to an object
		SmartSubPtr(IRefCount<Derived> * ptr)
		{
			this->__m_refcount = NULL;
			__Assign(ptr);
		}

		//!	Construct a sub pointer that takes its target from a smart pointer
		SmartSubPtr(const SmartPtr<Base> &sp)
		{
			this->__m_refcount = NULL;
			__Assign(sp.GetPtr());
		}

		//!	Construct a smart pointer that takes its target from another sub pointer
		SmartSubPtr(const SmartSubPtr<Base, Derived> &sp)
		{
			this->__m_refcount = NULL;
			__Assign(sp.__m_refcount);
		}

		//! Detatch this pointer from the object before destruction
		virtual ~SmartSubPtr()
		{
			__Assign((IRefCount<Base> *)NULL);
		}

		//! Get the contained pointer
		Derived *GetPtr() const
		{
			if(this->__m_refcount==NULL) return NULL;
			return dynamic_cast<Derived*>(this->__m_refcount->GetPtr());
		}

		//! Give const access to members of Derived
		const Derived * operator ->() const
		{
			mxflib_assert(GetPtr()!=NULL);
			return (const Derived*)GetPtr();
		}

		//! Give access to members of Derived
		Derived * operator ->()
		{
			mxflib_assert(GetPtr()!=NULL);
			return (Derived*)GetPtr();
		}

		//! Conversion to const Derived* (for function calls)
		operator Derived* () const
		{
			return GetPtr();
		}

		//! Assign another sub pointer
		SmartSubPtr & operator = (const SmartSubPtr<Base, Derived> &sp) {__Assign(sp.__m_refcount); return *this;}

		//! Assign pointer or NULL
		SmartSubPtr & operator = (IRefCount<Base> * ptr) {__Assign(ptr); return *this;}

	protected:
		//!	Assign a 'smart' object to this smart pointer
		/*!	This method is picked over __Assign(void *ptr)
		 *	if T implements IRefCount.
		 *	This allows some memory usage optimization
		 *	\note
		 *	Assigning NULL will detatch this pointer from the
		 *	current object
		 */
		virtual void __Assign(IRefCount<Base> *refcount)
		{
			PTRDEBUG( if(DebugName.size()) debug("%s changing from %p to %p\n", DebugName.c_str(), __m_refcount, refcount); )

			// Attach us to the new object first
			// This is important in case we are assigned to
			// the same thing we are already attatched to,
			// in which case detatching first could cause
			// our object to be deleted!!
			
			if(refcount!=NULL) 
			{
				refcount->__IncRefCount();
			}

			// Record what we were attached to
			IRefCount<Base> *oldref = this->__m_refcount;

			// Make the new attachment
			this->__m_refcount = refcount;

			// Break the old attachment
			if(oldref!=NULL) 
				oldref->__DecRefCount();
		}

	};
}

		 
//! Macro to give typecast functionality even in MSVC 6
#define SmartPtr_Cast(Ptr, Type) ( Ptr.Cast((Type*)NULL) )

#endif // MXFLIB__SMARTPTR_H
