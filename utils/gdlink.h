/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
// Generic doubly linked list

#ifndef DLINK_H
#define DLINK_H

#include <iostream>
#include <stdio.h>

template <class TTYPE>
class gdlist;
template <class TTYPE>
class gdlink;

//-------------------------------------------------------------------------
// gdlink -- link declarations
//-------------------------------------------------------------------------

/* links for the doubly-linked list */			
template <class TTYPE>					
class gdlink {							
public:	/* for debugging only */			
	friend class gdlist<TTYPE>;				
	friend class fxsortedlist;				
	friend void interact();				
	gdlink<TTYPE> *nextItem;					
	gdlink<TTYPE> *prevItem;	/*lsy*/				
	TTYPE e;						
	
	gdlink (TTYPE a,	
			gdlink<TTYPE> *n = 0,				
			gdlink<TTYPE> *p = 0)				
	{ e = a; nextItem = n; prevItem = p; }			
	
	void insert(gdlink<TTYPE> *n);			
	void append(gdlink<TTYPE> *n);			
	void remove();					
};

//-------------------------------------------------------------------------
// gdlist -- list declarations
//-------------------------------------------------------------------------

/* manage a doubly-linked list */			
template <class TTYPE>
class gdlist {							
public: /* for debugging only */			
	gdlink<TTYPE> *lastItem;				
	gdlink<TTYPE> *currItem;					
	gdlink<TTYPE> *marcItem;					
	long kount;						
	
	/*	public:	*/						
	gdlist() { lastItem = currItem = 0; kount = 0; } 	
	gdlist(TTYPE e)					
	{							
		currItem = 0; kount = 1;				
		lastItem = new gdlink<TTYPE>(e);  /*lsy*/		
		lastItem->nextItem = lastItem->prevItem = lastItem;			
	}							
	void checklast(const char* s)			
	{							
		if (lastItem && !(lastItem->nextItem))			
			printf("Got one: %s\n",s);		
	}							
	int isempty() { return lastItem == 0; }  /*lsy*/		
	void clear();					
	~gdlist<TTYPE>() { clear(); };			
	
	/* set current pointer to 0 */			
	void reset() { currItem = 0; }  /*lsy*/			
	
	/* mark current pointer position */	/*lsy*/		
	void mark() { marcItem = currItem; }  /*lsy*/		
	
	/* mark prevItem pointer position */	/*lsy*/		
	void markprev() /*lsy*/				
	{							
		if (currItem)						
		{							
			marcItem = currItem->prevItem;				
			if (marcItem == lastItem)				
				marcItem = 0;					
		}							
		else						
			marcItem = 0; /* or should it be = lastItem ? */	
	}							
	
	/* mark lastItem pointer position */	/*lsy*/		
	void marklast() { marcItem = lastItem; }  /*lsy*/		
	
	/* move current pointer to mark */	/*lsy*/		
	void tomark() { currItem = marcItem; }			
	
	/* move current pointer to mark */	/*lsy*/		
	void tomark(TTYPE &a) { currItem = marcItem; getmark(a); }	
	
	/* return object at current mark */  /*lsy*/		
	void getmark(TTYPE &a)				
	{							
		if (marcItem)						
			a = marcItem->e;					
		else						
			a = 0;					
	}							
	
	/* YOU'D BETTER KNOW WHAT YOU'RE DOING IF YOU USE */	
	/* getcurr() AND setcurr(), AND YOU'D BETTER BE */	
	/* CERTAIN THAT THE LIST DOESN'T CHANGE BETWEEN */	
	/* A getcurr() AND A SUBSEQUENT setcurr() */	
	/* get current pointer */	/*lsy*/			
	gdlink<TTYPE>* getcurr() { return currItem; }		
	
	/* set current pointer */	/*lsy*/			
	void setcurr(gdlink<TTYPE> *c) { currItem = c; }		
	
	/* move around list, leaving links */		
	int next(TTYPE &e);					
	int prev(TTYPE &e);					
	int current(TTYPE &e);				
	
	/* move around list, removing links */		
	int getnext(TTYPE &e); /*lsy*/			
	int getprev(TTYPE &e); /*lsy*/			
	
	/* add object/entity to beginning or end of list */
	void insert(const TTYPE e); /*lsy*/			
	void append(const TTYPE e); /*lsy*/			
	void add(const TTYPE e) { append(e); }  /* synonym */	
	
	/* add link to beginning or end of list */		
	void insert(gdlink<TTYPE>* e); /*lsy*/		
	void append(gdlink<TTYPE>* e); /*lsy*/		
	void add(gdlink<TTYPE>* e) { append(e); } /* synonym */
	
	/* add object/entity before or after current entry */
	void inserthere(const TTYPE e); /*lsy*/		
	void appendhere(const TTYPE e); /*lsy*/		
	
	/* add link before or after current entry */		
	void inserthere(gdlink<TTYPE>* e); /*lsy*/		
	void appendhere(gdlink<TTYPE>* e); /*lsy*/		
	
	/* remove the current entry */			
	void remove();					
	
	/* unlink & return the current entry */		
	gdlink<TTYPE>* unlink();				
	
	/* remove the entry corresponding to a */		
	void remove(const TTYPE a);				
	
	/* unlink & return the entry corresponding to a */	
	gdlink<TTYPE>* unlink(const TTYPE a);			
	
	/* unlink & return the entry a */			
	gdlink<TTYPE>* unlink(gdlink<TTYPE>* a);		
	
	/* return the number of elements in the list */	
	long count() { return kount; }			
};

//-------------------------------------------------------------------------
// gdlink -- link implementations
//-------------------------------------------------------------------------

/* insert a link in front of this one */		
template <class TTYPE>					
void gdlink<TTYPE>::insert(gdlink<TTYPE> *newlink)	
{							
	if (prevItem)					
		prevItem->nextItem = newlink;				
	newlink->nextItem = this;				
	newlink->prevItem = prevItem;				
	prevItem = newlink;					
}							

/* append a link after this one */			
template <class TTYPE>					
void gdlink<TTYPE>::append(gdlink<TTYPE> *newlink)	
{							
	if (nextItem)						
		nextItem->prevItem = newlink;				
	newlink->nextItem = nextItem;				
	newlink->prevItem = this;				
	nextItem = newlink;					
}							

/* remove this link from the list */			
template <class TTYPE>					
void gdlink<TTYPE>::remove()				
{							
	if (prevItem)						
		prevItem->nextItem = nextItem;				
	if (nextItem)						
		nextItem->prevItem = prevItem;				
	nextItem = prevItem = 0;					
}

//-------------------------------------------------------------------------
// gdlist -- list implementations
//-------------------------------------------------------------------------

/* clear the list */					
template <class TTYPE>					
void gdlist<TTYPE>::clear()				
{							
	gdlink<TTYPE> *l = lastItem;				
	if (!l) return;					
	
	do							
	{							
		gdlink<TTYPE> *ll = l;				
		l = l->nextItem;					
		delete ll;					
	} while (l != lastItem);					
	
	kount = 0;						
	lastItem = 0;						
}							

/* get the next entry from list, moving currItem forward */	
template <class TTYPE>					
int gdlist<TTYPE>::next(TTYPE &a)				
{							
	if (!lastItem) /* is empty */				
		return 0;						
	
	/* move currItem forward, possibly to beginning or end */
	if (currItem)
	{						
		if (currItem == lastItem)					
		{							
			currItem = 0;
			return 0;					
		}							
		else						
			currItem = currItem->nextItem;	
	}
	else							
		currItem = lastItem->nextItem;				
	
	a = currItem->e;
	return 1;						
}							

/* get previous entry from list, moving currItem backwards */
template <class TTYPE>					
int gdlist<TTYPE>::prev(TTYPE &a)				
{							
	if (!lastItem)						
		return 0;						
	
	/* move currItem backward, possibly to beginning or end */
	if (currItem)
	{
		if (currItem == lastItem->nextItem)				
		{							
			currItem = 0;					
			return 0;					
		}							
		else						
			currItem = currItem->prevItem;			
	}
	else							
		currItem = lastItem;					
	
	a = currItem->e;						
	return 1;						
}							

/* get the current entry from the list */		
template <class TTYPE>					
int gdlist<TTYPE>::current(TTYPE &a)			
{							
	if (!lastItem || !currItem) /* is empty or off end */	
		return 0;						
	
	a = currItem->e;						
	return 1;						
}							

/* get next entry, removing it after; currItem is unchanged */
template <class TTYPE>					
int gdlist<TTYPE>::getnext(TTYPE &a)			
{							
	if (!lastItem)						
		return 0;						
	
	/* choose the link to remove */			
	gdlink<TTYPE> *f;					
	if (currItem)
	{
		if (currItem == lastItem)					
		{							
			currItem = 0;					
			return 0; /*lsy*/				
		}							
		else						
			f = currItem->nextItem;			
	}
	else	// if we're off the end of the list, loop back to the beginning of the list
		f = lastItem->nextItem;					
	
	a = f->e;						
	
	/* remove f from the list */				
	if (f == lastItem)					
		lastItem = 0;						
	else							
		f->remove();					
	
	delete f;						
	kount--;						
	return 1;						
}							

/* get previous entry, remove it after; currItem unchanged */
template <class TTYPE>					
int gdlist<TTYPE>::getprev(TTYPE &a)			
{							
	if (!lastItem)						
		return 0;						
	
	/* choose the link to remove */
	gdlink<TTYPE> *f;
	if (currItem)
	{
		if (currItem == lastItem->prevItem)				
		{							
			currItem = 0; /*lsy*/				
			return 0;					
		}							
		else						
			f = currItem->prevItem;
	}
	else							
		currItem = lastItem;					
	
	a = f->e;						
	
	/* remove f from the list */				
	if (f == lastItem)					
		lastItem = 0;						
	else							
		f->remove();					
	
	delete f;						
	kount--;						
	return 1;						
}							

/* insert entry at beginning of list */			
template <class TTYPE>					
void gdlist<TTYPE>::insert(const TTYPE a)			
{							
	gdlink<TTYPE>* l = new gdlink<TTYPE>(a);
	
	if (lastItem)
	{
		lastItem->nextItem->insert(l);
	}
	else
	{
		lastItem = l;
		lastItem->nextItem = lastItem->prevItem = lastItem;
	}
	kount++;
}

/* insert link at beginning of list */			
template <class TTYPE>					
void gdlist<TTYPE>::insert(gdlink<TTYPE>* l)		
{							
	if (lastItem)						
		lastItem->nextItem->insert(l);				
	else							
	{							
		lastItem = l;						
		lastItem->nextItem = lastItem->prevItem = lastItem;			
	}							
	kount++;						
}							

/* append entry to end of list */			
/* note this is same as insert except lastItem is adjusted */
template <class TTYPE>					
void gdlist<TTYPE>::append(const TTYPE a)			
{							
	this->insert(a);					
	lastItem = lastItem->nextItem;					
}							

/* append link to end of list */			
/* note this is same as insert except lastItem is adjusted */
template <class TTYPE>					
void gdlist<TTYPE>::append(gdlink<TTYPE>* a)		
{							
	this->insert(a);					
	lastItem = lastItem->nextItem;					
}							

/* insert entry before current location in list */	
/* if currItem is not set, insert at beginning of list */	
template <class TTYPE>					
void gdlist<TTYPE>::inserthere(const TTYPE a)		
{							
	if (currItem)						
	{
		gdlink<TTYPE>* l = new gdlink<TTYPE>(a);
		
		currItem->insert(l);
		kount++;
	}
	else
	{
		this->insert(a);
	}
}

/* insert link before current location in list */	
/* if currItem is not set, insert at beginning of list */	
template <class TTYPE>					
void gdlist<TTYPE>::inserthere(gdlink<TTYPE>* l)		
{							
	if (currItem)						
	{							
		currItem->insert(l);					
		kount++;						
	}							
	else							
		this->insert(l);					
}							

/* append an entry after the current location in list */
/* if currItem is not set, append at end of list */		
template <class TTYPE>					
void gdlist<TTYPE>::appendhere(const TTYPE a)		
{							
	if (currItem)						
	{							
		currItem->append(new gdlink<TTYPE>(a));		
		kount++;						
	}							
	else							
		this->append(a);					
}							

/* append a link after the current location in list */	
/* if currItem is not set, append at end of list */		
template <class TTYPE>					
void gdlist<TTYPE>::appendhere(gdlink<TTYPE>* a)		
{							
	if (currItem)						
	{							
		currItem->append(a);					
		kount++;						
	}							
	else							
		this->append(a);					
}							

/* remove the current entry */				
template <class TTYPE>					
void gdlist<TTYPE>::remove()				
{							
	gdlink<TTYPE> *prevcurr;				
	if (isempty())					
		return;						
	if (currItem)						
	{							
		if (currItem == lastItem->nextItem) /* first link */		
		{
			currItem->remove();
			delete currItem;
			currItem = 0;
			kount--;
		}							
		else if (currItem == lastItem) /* lastItem link */		
		{
			prevcurr = currItem->prevItem;
			currItem->remove();
			delete currItem;
			currItem = prevcurr;
			lastItem = currItem;
			kount--;
		}							
		else						
		{
			prevcurr = currItem->prevItem;
			currItem->remove();
			delete currItem;
			currItem = prevcurr;
			kount--;
		}							
	}							
	else	 /* treat currItem == 0 like first link?  It might be much better to do nothing here */
	{							
		currItem = lastItem->nextItem; /* first link */		
		currItem->remove();					
		delete currItem;					
		currItem = 0;						
		kount--;						
	}							
	if (!kount)
		lastItem = 0;				
}							

/* unlink & return the current entry */			
template <class TTYPE>					
gdlink<TTYPE>* gdlist<TTYPE>::unlink()			
{							
	gdlink<TTYPE> *prevcurr;				
	gdlink<TTYPE> *savecurr;				
	if (isempty())					
		return 0;					
	if (currItem)						
	{							
		if (currItem == lastItem->nextItem) /* first link */		
		{							
			currItem->remove();				
			savecurr = currItem;				
			currItem = 0;					
			kount--;					
		}							
		else if (currItem == lastItem) /* lastItem link */		
		{							
			prevcurr = currItem->prevItem;				
			currItem->remove();				
			savecurr = currItem;				
			currItem = prevcurr;				
			lastItem = currItem;					
			kount--;					
		}							
		else						
		{							
			prevcurr = currItem->prevItem;				
			currItem->remove();				
			savecurr = currItem;				
			currItem = prevcurr;				
			kount--;					
		}							
	}							
	else	 /* treat currItem == 0 like first link?  It might be much better to do nothing here */
	{							
		currItem = lastItem->nextItem; /* first link */		
		currItem->remove();					
		savecurr = currItem;					
		currItem = 0;						
		kount--;						
	}							
	if (!kount)
		lastItem = 0;				
	return savecurr;					
}							

/* remove the entry corresponding to a */		
template <class TTYPE>					
void gdlist<TTYPE>::remove(const TTYPE a)			
{							
	gdlink<TTYPE> *savecurr;				
	savecurr = currItem;					
	currItem = 0;						
	TTYPE temp;						
	while (this->next(temp))				
	{							
		if (temp == a)					
		{							
			if (savecurr == currItem)
				savecurr = currItem->prevItem;	
			remove();					
			if (kount)					
				currItem = savecurr;				
			return;					
		}							
	}							
	std::cerr << "attempt to remove non-existent TTYPE node\n";
}							

/* unlink the entry corresponding to object a */	
template <class TTYPE>					
gdlink<TTYPE>* gdlist<TTYPE>::unlink(const TTYPE a)	
{							
	gdlink<TTYPE> *savecurr;				
	gdlink<TTYPE> *templink;				
	savecurr = currItem;					
	currItem = 0;						
	TTYPE temp;						
	while (this->next(temp))				
	{							
		if (temp == a)					
		{							
			if (savecurr == currItem)
				savecurr = currItem->prevItem;	
			templink = unlink();				
			if (kount)					
				currItem = savecurr;				
			return templink;				
		}							
	}							
	std::cerr << "attempt to remove non-existent TTYPE node\n";
}							

/* unlink the link corresponding to link a */		
template <class TTYPE>					
gdlink<TTYPE>* gdlist<TTYPE>::unlink(gdlink<TTYPE>* a)	
{							
	gdlink<TTYPE> *savecurr;				
	gdlink<TTYPE> *temp;					
	savecurr = currItem;					
	currItem = lastItem->nextItem; /* first link */			
	do							
	{							
		if (currItem == a)					
		{							
			if (currItem == savecurr)				
				savecurr = currItem->prevItem;			
			temp = unlink();				
			if (kount)					
				currItem = savecurr;				
			return temp;					
		}							
	} while ((currItem = currItem->nextItem) != lastItem->nextItem);		
	/* wrapped back around to first, so search failed */	
	std::cerr << "attempt to remove non-existent TTYPE node\n";
}

#endif DLINK_H
