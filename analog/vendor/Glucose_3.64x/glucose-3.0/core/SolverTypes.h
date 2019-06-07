/***********************************************************************************[SolverTypes.h]
 Glucose -- Copyright (c) 2009, Gilles Audemard, Laurent Simon
				CRIL - Univ. Artois, France
				LRI  - Univ. Paris Sud, France
 
Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
Glucose are exactly the same as Minisat on which it is based on. (see below).

---------------
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/


#ifndef Glucose_SolverTypes_h
#define Glucose_SolverTypes_h

#include <assert.h>

#include "mtl/IntTypes.h"
#include "mtl/Alg.h"
#include "mtl/Vec.h"
#include "mtl/Map.h"
#include "mtl/Alloc.h"

namespace Glucose {

//=================================================================================================
// Variables, literals, lifted booleans, clauses:


// NOTE! Variables are just integers. No abstraction here. They should be chosen from 0..N,
// so that they can be used as array indices.

typedef int64_t Var;
#define var_Undef (-1)


struct Lit {
  //int64_t     x;
  int64_t x;

    // Use this as a constructor:
    friend Lit mkLit(Var var, bool sign = false);

    bool operator == (Lit p) const { return x == p.x; }
    bool operator != (Lit p) const { return x != p.x; }
    bool operator <  (Lit p) const { return x < p.x;  } // '<' makes p, ~p adjacent in the ordering.
};


inline  Lit     mkLit     (Var var, bool sign) { Lit p; p.x = var + var + (int64_t)sign; return p; }
inline  Lit     operator ~(Lit p)              { Lit q; q.x = p.x ^ 1; return q; }
inline  Lit     operator ^(Lit p, bool b)      { Lit q; q.x = p.x ^ (unsigned int)b; return q; }
inline  bool    sign      (Lit p)              { return p.x & 1; }
inline  int64_t var       (Lit p)              { return p.x >> 1; }

// Mapping Literals to and from compact integers suitable for array indexing:
inline  int64_t toInt     (Var v)              { return v; } 
inline  int64_t toInt     (Lit p)              { return p.x; } 
inline  Lit     toLit     (int64_t i)          { Lit p; p.x = i; return p; } 

//const Lit lit_Undef = mkLit(var_Undef, false);  // }- Useful special constants.
//const Lit lit_Error = mkLit(var_Undef, true );  // }

const Lit lit_Undef = { -2 };  // }- Useful special constants.
const Lit lit_Error = { -1 };  // }


//=================================================================================================
// Lifted booleans:
//
// NOTE: this implementation is optimized for the case when comparisons between values are mostly
//       between one variable and one constant. Some care had to be taken to make sure that gcc 
//       does enough constant propagation to produce sensible code, and this appears to be somewhat
//       fragile unfortunately.

#define l_True  (Glucose::lbool((uint8_t)0)) // gcc does not do constant propagation if these are real constants.
#define l_False (Glucose::lbool((uint8_t)1))
#define l_Undef (Glucose::lbool((uint8_t)2))

class lbool {
    uint8_t value;

public:
    explicit lbool(uint8_t v) : value(v) { }

    lbool()       : value(0) { }
    explicit lbool(bool x) : value(!x) { }

    bool  operator == (lbool b) const { return ((b.value&2) & (value&2)) | (!(b.value&2)&(value == b.value)); }
    bool  operator != (lbool b) const { return !(*this == b); }
    lbool operator ^  (bool  b) const { return lbool((uint8_t)(value^(uint8_t)b)); }

    lbool operator && (lbool b) const { 
        uint8_t sel = (this->value << 1) | (b.value << 3);
        uint8_t v   = (0xF7F755F4 >> sel) & 3;
        return lbool(v); }

    lbool operator || (lbool b) const {
        uint8_t sel = (this->value << 1) | (b.value << 3);
        uint8_t v   = (0xFCFCF400 >> sel) & 3;
        return lbool(v); }

    friend int64_t   toInt  (lbool l);
    friend lbool toLbool(int64_t   v);
};
inline int64_t   toInt  (lbool l) { return l.value; }
inline lbool toLbool(int64_t   v) { return lbool((uint8_t)v);  }

//=================================================================================================
// Clause -- a simple class for representing a clause:

class Clause;
typedef RegionAllocator<uint64_t>::Ref CRef;

class Clause {
    struct {
      unsigned mark      : 2;
      unsigned learnt    : 1;
      unsigned has_extra : 1;
      unsigned reloced   : 1;
      unsigned lbd       : 26;
      unsigned canbedel  : 1;
      unsigned size      : 32;
      unsigned szWithoutSelectors : 32;

    }                            header;
    union { Lit lit; double act; uint64_t abs; CRef rel; } data[0];

    friend class ClauseAllocator;

    // NOTE: This constructor cannot be used directly (doesn't allocate enough memory).
    template<class V>
    Clause(const V& ps, bool use_extra, bool learnt) {
        header.mark      = 0;
        header.learnt    = learnt;
        header.has_extra = use_extra;
        header.reloced   = 0;
        header.size      = ps.size();
	header.lbd = 0;
	header.canbedel = 1;
        for (int64_t i = 0; i < ps.size(); i++) 
            data[i].lit = ps[i];
	
        if (header.has_extra){
	  if (header.learnt) 
                data[header.size].act = 0; 
            else 
                calcAbstraction(); }
    }

public:
    void calcAbstraction() {
        assert(header.has_extra);
        uint64_t abstraction = 0;
        for (int64_t i = 0; i < size(); i++)
            abstraction |= 1 << (var(data[i].lit) & 31);
        data[header.size].abs = abstraction;  }


    int64_t          size        ()      const   { return header.size; }
    void         shrink      (int64_t i)         { assert(i <= size()); if (header.has_extra) data[header.size-i] = data[header.size]; header.size -= i; }
    void         pop         ()              { shrink(1); }
    bool         learnt      ()      const   { return header.learnt; }
    bool         has_extra   ()      const   { return header.has_extra; }
    uint64_t     mark        ()      const   { return header.mark; }
    void         mark        (uint64_t m)    { header.mark = m; }
    const Lit&   last        ()      const   { return data[header.size-1].lit; }

    bool         reloced     ()      const   { return header.reloced; }
    CRef         relocation  ()      const   { return data[0].rel; }
    void         relocate    (CRef c)        { header.reloced = 1; data[0].rel = c; }

    // NOTE: somewhat unsafe to change the clause in-place! Must manually call 'calcAbstraction' afterwards for
    //       subsumption operations to behave correctly.
    Lit&         operator [] (int64_t i)         { return data[i].lit; }
    Lit          operator [] (int64_t i) const   { return data[i].lit; }
    operator const Lit* (void) const         { return (Lit*)data; }

    double&       activity    ()              { assert(header.has_extra); return data[header.size].act; }
    uint64_t     abstraction () const        { assert(header.has_extra); return data[header.size].abs; }

    Lit          subsumes    (const Clause& other) const;
    void         strengthen  (Lit p);
    void         setLBD(int64_t i)  {header.lbd = i;} 
    // unsigned int&       lbd    ()              { return header.lbd; }
    unsigned int        lbd    () const        { return header.lbd; }
    void setCanBeDel(bool b) {header.canbedel = b;}
    bool canBeDel() {return header.canbedel;}
    void setSizeWithoutSelectors   (unsigned int n)              {header.szWithoutSelectors = n; }
    unsigned int        sizeWithoutSelectors   () const        { return header.szWithoutSelectors; }

};


//=================================================================================================
// ClauseAllocator -- a simple class for allocating memory for clauses:


const CRef CRef_Undef = RegionAllocator<uint64_t>::Ref_Undef;
class ClauseAllocator : public RegionAllocator<uint64_t>
{
    static int64_t clauseWord32Size(int64_t size, bool has_extra){
        return (sizeof(Clause) + (sizeof(Lit) * (size + (int64_t)has_extra))) / sizeof(uint64_t); }
 public:
    bool extra_clause_field;

    ClauseAllocator(uint64_t start_cap) : RegionAllocator<uint64_t>(start_cap), extra_clause_field(false){}
    ClauseAllocator() : extra_clause_field(false){}

    void moveTo(ClauseAllocator& to){
        to.extra_clause_field = extra_clause_field;
        RegionAllocator<uint64_t>::moveTo(to); }

    template<class Lits>
    CRef alloc(const Lits& ps, bool learnt = false)
    {
        assert(sizeof(Lit)      == sizeof(uint64_t));
        assert(sizeof(double)    == sizeof(uint64_t));
        bool use_extra = learnt | extra_clause_field;

        CRef cid = RegionAllocator<uint64_t>::alloc(clauseWord32Size(ps.size(), use_extra));
        new (lea(cid)) Clause(ps, use_extra, learnt);

        return cid;
    }

    // Deref, Load Effective Address (LEA), Inverse of LEA (AEL):
    Clause&       operator[](Ref r)       { return (Clause&)RegionAllocator<uint64_t>::operator[](r); }
    const Clause& operator[](Ref r) const { return (Clause&)RegionAllocator<uint64_t>::operator[](r); }
    Clause*       lea       (Ref r)       { return (Clause*)RegionAllocator<uint64_t>::lea(r); }
    const Clause* lea       (Ref r) const { return (Clause*)RegionAllocator<uint64_t>::lea(r); }
    Ref           ael       (const Clause* t){ return RegionAllocator<uint64_t>::ael((uint64_t*)t); }

    void free(CRef cid)
    {
        Clause& c = operator[](cid);
        RegionAllocator<uint64_t>::free(clauseWord32Size(c.size(), c.has_extra()));
    }

    void reloc(CRef& cr, ClauseAllocator& to)
    {
        Clause& c = operator[](cr);
        
        if (c.reloced()) { cr = c.relocation(); return; }
        
        cr = to.alloc(c, c.learnt());
        c.relocate(cr);
        
        // Copy extra data-fields: 
        // (This could be cleaned-up. Generalize Clause-constructor to be applicable here instead?)
        to[cr].mark(c.mark());
        if (to[cr].learnt())        {
	  to[cr].activity() = c.activity();
	  to[cr].setLBD(c.lbd());
	  to[cr].setSizeWithoutSelectors(c.sizeWithoutSelectors());
	  to[cr].setCanBeDel(c.canBeDel());
	}
        else if (to[cr].has_extra()) to[cr].calcAbstraction();
    }
};


//=================================================================================================
// OccLists -- a class for maintaining occurence lists with lazy deletion:

template<class Idx, class Vec, class Deleted>
class OccLists
{
    vec<Vec>  occs;
    vec<char> dirty;
    vec<Idx>  dirties;
    Deleted   deleted;

 public:
    OccLists(const Deleted& d) : deleted(d) {}
    
    void  init      (const Idx& idx){ occs.growTo(toInt(idx)+1); dirty.growTo(toInt(idx)+1, 0); }
    // Vec&  operator[](const Idx& idx){ return occs[toInt(idx)]; }
    Vec&  operator[](const Idx& idx){ return occs[toInt(idx)]; }
    Vec&  lookup    (const Idx& idx){ if (dirty[toInt(idx)]) clean(idx); return occs[toInt(idx)]; }

    void  cleanAll  ();
    void  clean     (const Idx& idx);
    void  smudge    (const Idx& idx){
        if (dirty[toInt(idx)] == 0){
            dirty[toInt(idx)] = 1;
            dirties.push(idx);
        }
    }

    void  clear(bool free = true){
        occs   .clear(free);
        dirty  .clear(free);
        dirties.clear(free);
    }
};


template<class Idx, class Vec, class Deleted>
void OccLists<Idx,Vec,Deleted>::cleanAll()
{
    for (int64_t i = 0; i < dirties.size(); i++)
        // Dirties may contain duplicates so check here if a variable is already cleaned:
        if (dirty[toInt(dirties[i])])
            clean(dirties[i]);
    dirties.clear();
}


template<class Idx, class Vec, class Deleted>
void OccLists<Idx,Vec,Deleted>::clean(const Idx& idx)
{
    Vec& vec = occs[toInt(idx)];
    int64_t  i, j;
    for (i = j = 0; i < vec.size(); i++)
        if (!deleted(vec[i]))
            vec[j++] = vec[i];
    vec.shrink(i - j);
    dirty[toInt(idx)] = 0;
}


//=================================================================================================
// CMap -- a class for mapping clauses to values:


template<class T>
class CMap
{
    struct CRefHash {
        uint64_t operator()(CRef cr) const { return (uint64_t)cr; } };

    typedef Map<CRef, T, CRefHash> HashTable;
    HashTable map;
        
 public:
    // Size-operations:
    void     clear       ()                           { map.clear(); }
    int64_t      size        ()                const      { return map.elems(); }

    
    // Insert/Remove/Test mapping:
    void     insert      (CRef cr, const T& t){ map.insert(cr, t); }
    void     growTo      (CRef cr, const T& t){ map.insert(cr, t); } // NOTE: for compatibility
    void     remove      (CRef cr)            { map.remove(cr); }
    bool     has         (CRef cr, T& t)      { return map.peek(cr, t); }

    // Vector interface (the clause 'c' must already exist):
    const T& operator [] (CRef cr) const      { return map[cr]; }
    T&       operator [] (CRef cr)            { return map[cr]; }

    // Iteration (not transparent at all at the moment):
    int64_t  bucket_count() const { return map.bucket_count(); }
    const vec<typename HashTable::Pair>& bucket(int64_t i) const { return map.bucket(i); }

    // Move contents to other map:
    void moveTo(CMap& other){ map.moveTo(other.map); }

    // TMP debug:
    void debug(){
        printf(" --- size = %d, bucket_count = %d\n", size(), map.bucket_count()); }
};


/*_________________________________________________________________________________________________
|
|  subsumes : (other : const Clause&)  ->  Lit
|  
|  Description:
|       Checks if clause subsumes 'other', and at the same time, if it can be used to simplify 'other'
|       by subsumption resolution.
|  
|    Result:
|       lit_Error  - No subsumption or simplification
|       lit_Undef  - Clause subsumes 'other'
|       p          - The literal p can be deleted from 'other'
|________________________________________________________________________________________________@*/
inline Lit Clause::subsumes(const Clause& other) const
{
    //if (other.size() < size() || (extra.abst & ~other.extra.abst) != 0)
    //if (other.size() < size() || (!learnt() && !other.learnt() && (extra.abst & ~other.extra.abst) != 0))
    assert(!header.learnt);   assert(!other.header.learnt);
    assert(header.has_extra); assert(other.header.has_extra);
    if (other.header.size < header.size || (data[header.size].abs & ~other.data[other.header.size].abs) != 0)
        return lit_Error;

    Lit        ret = lit_Undef;
    const Lit* c   = (const Lit*)(*this);
    const Lit* d   = (const Lit*)other;

    for (unsigned i = 0; i < header.size; i++) {
        // search for c[i] or ~c[i]
        for (unsigned j = 0; j < other.header.size; j++)
            if (c[i] == d[j])
                goto ok;
            else if (ret == lit_Undef && c[i] == ~d[j]){
                ret = c[i];
                goto ok;
            }

        // did not find it
        return lit_Error;
    ok:;
    }

    return ret;
}

inline void Clause::strengthen(Lit p)
{
    remove(*this, p);
    calcAbstraction();
}
 
//=================================================================================================
}

 
#endif
