/*! \file	smartptr.h
 *	\brief	Smart Pointer class
 *
 *			This file contains the SmartPtr class (and helpers) originally
 *			written by Sandu Turcan and submitted to www.codeproject.com
 */
/*
 *	Important Note: 
 *	This code originally comes from www.codeproject.com
 *	and was written by Sandu Turcan (idlsoft@hotmail.com)
 *	The code has been modified and in this version is
 *	covered by the terms of the following licence:
 *
 *	Copyright (c) 2002, Matt Beard
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

//#define PTRDEBUG( x ) x
#define PTRDEBUG( x )


// Ensure we know NULL
#include <stdlib.h>

namespace mxflib 
{
	// Forward declaration of SmartPtr to allow it to be befreinded
	template <class T> class SmartPtr;


	//! An interface for reference counting
	/*! Classes may implement it themselves, or SmartPtr will
	 *  provide its internal implementation of IRefCount
	 */
	template <class T> class IRefCount 
	{
		// Allow SmartPtr access to our internals
		friend class SmartPtr<T>;

	protected:
		// A number of pure virtual functions that the reference counter needs
		
		virtual void __IncRefCount() = 0;	//!< Increment the number of references
		virtual void __DecRefCount() = 0;	//!< Decrement the number of references, if none left delete the object
		virtual T * GetPtr() const = 0;		//!< Get a pointer to the object
	};
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
    private:
		int __m_counter;					//!< The actual reference count

	protected:
		//! Increment the number of references
		virtual void __IncRefCount() 
		{
			__m_counter++;

			PTRDEBUG( debug("0x%08x Increment count -> %d\n", (int)this, __m_counter); )
		}

		//! Decrement the number of references, if none left delete the object
		virtual void __DecRefCount()
		{
			__m_counter--;

			PTRDEBUG( debug("0x%08x Decrement count -> %d\n", (int)this, __m_counter); )

			if(__m_counter<=0)
			{
				PTRDEBUG( debug("0x%08x Destroying\n", this); )

				__DestroyRef();
			}
		}

		//! Get a pointer to the object
		virtual T * GetPtr() const
		{
			return ((T *)this);
		}

		//! Destroy the object
		/*! Called once all references have gone
		 */
		virtual void __DestroyRef() 
		{ 
			if(GetPtr()!=NULL)
				delete GetPtr();
		}

    protected:
		//! Constructor for the RefCount class
		RefCount()
		{
			// No references yet!
			__m_counter = 0;

			PTRDEBUG( debug("0x%08x Build new (zero) count\n", (int)this); )
		}
	};
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
	 *	// ptr1 = &o1;  // DON'T ! only memory allocated by new operator should be used
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
	private:
		IRefCount<T> *__m_refcount;		//!< Pointer to the reference counted object

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
			virtual void __DestroyRef() { delete this; }

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
		void __Assign(void *ptr)
		{
			if(ptr==NULL)
				__Assign((IRefCount<T> *)NULL);
			else
			{
				__Assign(new __RefCounter(static_cast<T *>(ptr)));
			}
		}
		
		//!	Assign a 'smart' object to this smart pointer
		/*!	This method is picked over __Assign(void *ptr)
		 *	if T implements IRefCount.
		 *	This allows some memory usage optimization
		 *	/note
		 *	Assigning NULL will detatch this pointer from the
		 *	current object
		 */
		void __Assign(IRefCount<T> *refcount)
		{
			// Attach us to the new object first
			// This is important in case we are assigned to
			// the same thing we are already attatched to,
			// in which case detatching first could cause
			// our object to be deleted!!
			if(refcount!=NULL) refcount->__IncRefCount();

			// Record what we were attached to
			IRefCount<T> *oldref = __m_refcount;

			// Make the new attachment
			__m_refcount = refcount;

			// Break the old attachment
			if(oldref!=NULL) oldref->__DecRefCount();
		}

	public:
		//! Construct a smart pointer that points to nothing
		SmartPtr()
		{
			__m_refcount = NULL;
		}

		//!	Construct a smart pointer to an object
		SmartPtr(T * ptr)
		{
			__m_refcount = NULL;
			__Assign(ptr);
		}

		//!	Construct a smart pointer that takes its target from another smart pointer
		SmartPtr(const SmartPtr &sp)
		{
			__m_refcount = NULL;
			__Assign(sp.__m_refcount);
		}

		//! Detatch this pointer from the object before destruction
		virtual ~SmartPtr()
		{
			__Assign((IRefCount<T> *)NULL);
		}

		//! Get the contained pointer, not really needed but...
		T *GetPtr() const
		{
			if(__m_refcount==NULL) return NULL;
			return __m_refcount->GetPtr();
		}

		//! Assign another smart pointer
		SmartPtr & operator = (const SmartPtr &sp) {__Assign(sp.__m_refcount); return *this;}

		//! Assign pointer or NULL
		SmartPtr & operator = (T * ptr) {__Assign(ptr); return *this;}

		//! Give access to members of T
		T * operator ->()
		{
	        ASSERT(GetPtr()!=NULL);
	        return GetPtr();
	    }

		//! Give const access to members of T
		const T * operator ->() const
		{
	        ASSERT(GetPtr()!=NULL);
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
	};
}

#endif MXFLIB__SMARTPTR_H
