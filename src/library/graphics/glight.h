/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// glight.h: declarations of graphic light classes

#ifndef GLIGHT_H
#define GLIGHT_H

// Sytem
#include <gl.h>

class indexlist;
class gobject;

class glight // graphical light
{
public:
    glight();
	~glight();
    
    void settranslation(float x, float y, float z, float w);
    void settranslation(float x, float y = 0.0, float z = 0.0);
	void settranslation(float* pp);
	
    void setcol(float r, float g, float b);        
	void setcol(float* pcol);
    
    void setambient(float r, float g, float b);
    void setname(char* pc);
    
    void setdims(int d);
    void bind(short lightnum);
    void Use(short lightnum);
	void Draw();
	void Print();
    
	void translate();
	void position();	    
	float* getposptr();

private:
    static indexlist* pindices;
    short index; 		// index by which the iris will know this light definition
    float pos[4];
    float col[4]; 		// only 3 apply to lighting, 4th is for translucent draw
    float ambient[3]; 	// not really needed except for print
    gobject* pattobj;
    char* name;
    int dims;
};

inline void glight::settranslation(float x, float y, float z) { settranslation(x, y, z, pos[3]); }
inline void glight::settranslation(float* pp) { settranslation(pp[0], pp[1], pp[2], pos[3]); }
inline void glight::setcol(float* pcol) { setcol(pcol[0], pcol[1], pcol[2]); }
inline void glight::setname(char* pc) { name = pc; }
inline void glight::translate() { glTranslatef(pos[0], pos[1], pos[2]); }
inline float* glight::getposptr() { return &pos[0]; }



class glightmodel // graphical lighting model
{
public:
    glightmodel();
    ~glightmodel();
    
    void setambient(float r, float g, float b);
    void setlocalviewer(bool lv);
    void setattenuation(float fixed, float variable);
    inline void setname(char* pc);
	void bind();
	void Use();
    void print();
        
private:
    static indexlist* pindices;
    
    short index; 			// index by which the iris will know this lighting-model
    float ambient[3]; 		// not really needed except for print
    float localviewer; 		// not really needed except for print
    float attenuation[2]; 	// not really needed except for print
    char* name;

};

inline void glightmodel::setname(char* pc) { name = pc; }



#endif

