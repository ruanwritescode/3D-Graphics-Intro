//  CSCIx229 library
//  Willem A. (Vlakkies) Schreuder
#include "CSCIx229.h"

//
//  Set projection
//
void Project(double fov,double asp,double dim,int mode)
{
   //  Tell OpenGL we want to manipulate the projection matrix
   glMatrixMode(GL_PROJECTION);
   //  Undo previous transformations
   glLoadIdentity();
   //  Perspective transformation
   if (mode == 2) // Orthographic projection
      glOrtho(-asp*1.5*dim,+asp*1.5*dim, -1.5*dim,+1.5*dim, .4,10*dim);
   else //Perspective Projection
      gluPerspective(fov,asp,.001,50*dim);
   //  Switch to manipulating the model matrix
   glMatrixMode(GL_MODELVIEW);
   //  Undo previous transformations
   glLoadIdentity();
}

